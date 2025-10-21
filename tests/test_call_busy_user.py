import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def busy_user_b_scenario(client, handler):
    """User B waits for call from C and accepts"""
    if handler.wait_for_event("incoming_call_user_c", 10):
        print(f"[{handler.name}] Accepting call from user_c")
        client.accept_call("user_c")
        handler.wait_for_event("accept_result_OK", 5)


def busy_user_c_scenario(client, handler):
    """User C calls B"""
    print(f"[{handler.name}] Calling user_b")
    client.start_calling("user_b")
    handler.wait_for_event("call_result_OK", 10)


def busy_user_a_scenario(client, handler):
    """User A waits and then calls B who should be busy"""
    time.sleep(4)
    print(f"[{handler.name}] Calling user_b (should be busy)")
    client.start_calling("user_b")
    time.sleep(2)


class CallBusyUserTest(TestRunner):
    def test_call_busy_user(self):
        """A calls B who is already in call with C"""
        b_handler = CallbacksHandler("user_b")
        c_handler = CallbacksHandler("user_c")
        a_handler = CallbacksHandler("user_a")
        
        b_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_b", b_handler, busy_user_b_scenario, 8)
        )
        
        c_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_c", c_handler, busy_user_c_scenario)
        )
        
        a_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user_a", a_handler, busy_user_a_scenario, 8)
        )
        
        b_process.start()
        time.sleep(1)
        c_process.start()
        time.sleep(2)
        a_process.start()
        
        a_process.join(timeout=15)
        b_process.join(timeout=15)
        c_process.join(timeout=15)
        
        for p in [a_process, b_process, c_process]:
            if p.is_alive():
                p.terminate()
        
        bc_success = ("call_result_OK" in c_handler.events and
                     "incoming_call_user_c" in b_handler.events)
        
        return bc_success

if __name__ == "__main__":
    test = CallBusyUserTest("8081")
    if test.start_server():
        try:
            result = test.test_call_busy_user()
            print(f"Call busy user test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()