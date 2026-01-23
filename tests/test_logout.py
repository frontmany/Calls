import sys
import time
import multiprocessing

from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner


def logout_scenario(client, handler):
    """Scenario: authorize then logout"""
    print(f"[{handler.name}] Authorized, now logging out")
    time.sleep(1)
    client.logout()
    handler.wait_for_event("logout_completed", 5)


class LogoutTest(TestRunner):
    def test_logout(self):
        """Test user logout"""
        events = multiprocessing.Manager().list()
        
        process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "logout_user", events, "logout_user", logout_scenario, 5)
        )
        
        process.start()
        process.join(timeout=10)
        
        if process.is_alive():
            process.terminate()
            return False
        
        return "logout_completed" in events


if __name__ == "__main__":
    test = LogoutTest("8081")
    if test.start_server():
        try:
            result = test.test_logout()
            print(f"Logout test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
