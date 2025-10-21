# Calls System Test Suite

This directory contains comprehensive tests for the Calls system.

## Test Files

### Infrastructure
- **test_runner_base.py** - Base infrastructure for all tests (CallbacksHandler, TestRunner, run_client, run_client_flexible)
- **tests.py** - Main test runner with test discovery and execution

### Documentation
- **README_TESTS.md** - This file, main test suite documentation
- **FLEXIBLE_CLIENT_EXAMPLES.md** - Detailed examples and usage guide for the flexible client runner
- **REFACTORING_SUMMARY.md** - Summary of the refactoring changes and migration guide
- **test_flexible_example.py** - Working examples demonstrating the flexible approach

### Basic Tests
- **test_authorization.py** - Test single user authorization
- **test_call_flow.py** - Test complete call flow between two clients
- **test_incoming_call_accept.py** - Test accepting incoming calls
- **test_incoming_call_decline.py** - Test declining incoming calls
- **test_audio_controls.py** - Test audio controls (mute, volume)

### Advanced Tests
- **test_accept_while_calling.py** - Test accepting calls while already calling
- **test_call_busy_user.py** - Test calling a busy user
- **test_call_queue.py** - Test incoming call queue handling
- **test_overlapping_calls.py** - Test multiple overlapping calls between users
- **test_rapid_redial.py** - Test rapid redial after declined calls
- **test_triple_call.py** - Test call chain scenarios

## Running Tests

### Run Only Basic Tests
```bash
python tests.py
```
or
```bash
python tests.py --basic
```

This runs the following basic tests:
- Authorization
- Call Flow
- Incoming Call Accept
- Incoming Call Decline
- Audio Controls

### Run All Tests (Including Advanced Tests)
```bash
python tests.py --all
```

This will:
1. Run all basic tests from tests.py
2. Automatically discover and run tests from all test_*.py files
3. Display a comprehensive test report

### Run Individual Test Files
Each test file can be run independently:

**Basic Tests:**
```bash
python test_authorization.py
python test_call_flow.py
python test_incoming_call_accept.py
python test_incoming_call_decline.py
python test_audio_controls.py
```

**Advanced Tests:**
```bash
python test_accept_while_calling.py
python test_call_busy_user.py
python test_call_queue.py
python test_overlapping_calls.py
python test_rapid_redial.py
python test_triple_call.py
```

## Test Architecture

### TestRunner Base Class
All test classes inherit from `TestRunner` which provides:
- `start_server()` - Starts the Calls server
- `stop_server()` - Stops the Calls server
- `run_test(test_func, test_name)` - Runs a single test with formatted output

### CallbacksHandler
Thread-safe handler that collects all callbacks from the client and stores events in a shared list that can be checked by tests. Provides `wait_for_event()` method for waiting for specific events with timeout.

### run_client (Legacy)
Helper function to run a client in a separate process with a **predefined** test scenario ("caller", "responder_accept", etc.). Maintained for backward compatibility.

### run_client_flexible (New)
**Flexible** client runner that accepts custom scenario functions. This is the recommended approach for new tests.

```python
def custom_scenario(client, handler):
    client.start_calling("friend_user")
    handler.wait_for_event("call_result_OK", 10)

run_client_flexible("localhost", "8081", "my_user", handler, custom_scenario)
```

For detailed examples, see `FLEXIBLE_CLIENT_EXAMPLES.md`.

## Creating New Tests

### Recommended Approach (Flexible)

Use the new `run_client_flexible()` for maximum flexibility:

1. Import the base infrastructure:
```python
from test_runner_base import CallbacksHandler, run_client_flexible, TestRunner
import multiprocessing
```

2. Create a test class with custom scenarios:
```python
class MyNewTest(TestRunner):
    def test_my_scenario(self):
        # Define custom scenario function
        def user1_scenario(client, handler):
            client.start_calling("user2")
            handler.wait_for_event("call_result_OK", 10)
        
        def user2_scenario(client, handler):
            if handler.wait_for_event("incoming_call_user1", 10):
                client.accept_call("user1")
        
        # Create handlers
        user1_handler = CallbacksHandler("User1")
        user2_handler = CallbacksHandler("User2")
        
        # Run clients in separate processes
        user2_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user2", user2_handler, user2_scenario)
        )
        user1_process = multiprocessing.Process(
            target=run_client_flexible,
            args=("localhost", self.port, "user1", user1_handler, user1_scenario)
        )
        
        user2_process.start()
        time.sleep(2)
        user1_process.start()
        
        user1_process.join(timeout=15)
        user2_process.join(timeout=15)
        
        # Cleanup
        for p in [user1_process, user2_process]:
            if p.is_alive():
                p.terminate()
        
        # Check results
        return "call_result_OK" in user1_handler.events
```

3. Add a main block:
```python
if __name__ == "__main__":
    test = MyNewTest("8081")
    if test.start_server():
        try:
            result = test.test_my_scenario()
            print(f"My test: {'PASSED' if result else 'FAILED'}")
        finally:
            test.stop_server()
```

4. The test will be automatically discovered when running `python tests.py --all`

### Legacy Approach

For backward compatibility, you can still use `run_client()` with predefined scenarios:
```python
from test_runner_base import CallbacksHandler, run_client, TestRunner

# Use predefined scenarios: "caller", "responder_accept", "responder_decline", "audio_test"
run_client("localhost", "8081", "caller_user", handler, "caller")
```

**Note:** The flexible approach is recommended for new tests as it provides:
- No hardcoded user names or patterns
- Custom logic with conditions and loops
- Better testability of complex scenarios

See `test_flexible_example.py` for complete working examples.

## Requirements

- Python 3.x
- callsClientPy module (compiled from C++)
- callsServerPy module (compiled from C++)
- Modules should be located at: `C:/prj/C++/Calls/build/clientPy/Release` and `C:/prj/C++/Calls/build/serverPy/Release`

## Platform Compatibility

✅ **Windows Compatible** - All tests have been refactored to work with Windows multiprocessing (spawn method)

The test system uses `multiprocessing` to run tests in parallel. On Windows, this requires all functions passed to processes to be defined at module level (not as nested functions). All test files follow this requirement.

For details on the Windows compatibility implementation, see `WINDOWS_FIX_SUMMARY.md`.

