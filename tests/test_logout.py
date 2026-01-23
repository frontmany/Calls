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
        from test_runner_base import generate_unique_nickname
        events = multiprocessing.Manager().list()
        unique_nickname = generate_unique_nickname("logout_user")
        
        process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, unique_nickname, events, "logout_user", logout_scenario, 5)
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
