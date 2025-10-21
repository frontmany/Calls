import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def queue_popular_user_scenario(client, handler):
    """Accept first call, decline others"""
    accepted_count = 0
    for i in range(40):
        for event in list(handler.events):
            if event.startswith("incoming_call_caller_"):
                caller = event.replace("incoming_call_", "")
                if accepted_count == 0:
                    print(f"[{handler.name}] Accepting call from {caller}")
                    client.accept_call(caller)
                    accepted_count += 1
                else:
                    print(f"[{handler.name}] Declining call from {caller}")
                    client.decline_call(caller)
        time.sleep(0.25)


def queue_caller_scenario(client, handler):
    """Caller scenario for queue test"""
    time.sleep(2)
    print(f"[{handler.name}] Calling popular_user")
    client.start_calling("popular_user")
    handler.wait_for_event("call_result_OK", 10)


class CallQueueTest(TestRunner):
    def test_incoming_call_queue(self):
        """Multiple users call one user, testing queue"""
        popular_handler = CallbacksHandler("popular_user")
        callers = [f"caller_{i}" for i in range(3)]
        
        popular_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "popular_user", popular_handler, queue_popular_user_scenario, 10)
        )
        popular_process.start()
        time.sleep(2)
        
        caller_processes = []
        caller_handlers = []
        
        for caller in callers:
            handler = CallbacksHandler(caller)
            caller_handlers.append(handler)
            
            process = multiprocessing.Process(
                target=run_client_flexible,
                args=("localhost", self.port, caller, handler, queue_caller_scenario)
            )
            caller_processes.append(process)
            process.start()
            time.sleep(1)
        
        popular_process.join(timeout=20)
        for process in caller_processes:
            process.join(timeout=10)
        
        if popular_process.is_alive():
            popular_process.terminate()
        for process in caller_processes:
            if process.is_alive():
                process.terminate()
        
        popular_got_calls = any(event.startswith("incoming_call_caller_") 
                               for event in popular_handler.events)
        call_attempts = sum(1 for handler in caller_handlers 
                          if "call_result_OK" in handler.events or "auth_result_OK" in handler.events)
        
        return popular_got_calls and call_attempts >= 2

if __name__ == "__main__":
    test = CallQueueTest("8081")
    if test.start_server():
        try:
            result = test.test_incoming_call_queue()
            print(f"Call queue test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()