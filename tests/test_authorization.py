import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def auth_only_scenario(client, handler):
    """Just authorize and wait, no additional actions needed"""
    pass


class AuthorizationTest(TestRunner):
    def test_authorization(self):
        """Test single user authorization"""
        from test_runner_base import generate_unique_nickname
        events_list = multiprocessing.Manager().list()
        unique_nickname = generate_unique_nickname("test_user")
        
        process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, unique_nickname, events_list, "auth_test", auth_only_scenario)
        )
        
        process.start()
        process.join(timeout=10)
        
        if process.is_alive():
            process.terminate()
            return False
        
        return "auth_result_OK" in events_list


if __name__ == "__main__":
    test = AuthorizationTest("8081")
    if test.start_server():
        try:
            result = test.test_authorization()
            print(f"Authorization test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()

