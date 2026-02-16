import time
import threading
import multiprocessing
import sys
import os
import uuid
import random
from functools import partial

# Project root (parent of tests/)
_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_OUT_RELEASE = os.path.join(_PROJECT_ROOT, 'out', 'build', 'x64-Release')
_OUT_DEBUG = os.path.join(_PROJECT_ROOT, 'out', 'build', 'x64-Debug')

# Paths to compiled Python modules (.pyd лежит прямо в этих папках)
possible_client_paths = [
    os.path.join(_OUT_RELEASE, 'clientCorePythonWrapper'),
    os.path.join(_OUT_DEBUG, 'clientCorePythonWrapper'),
]

possible_server_paths = [
    os.path.join(_OUT_RELEASE, 'serverPythonWrapper'),
    os.path.join(_OUT_DEBUG, 'serverPythonWrapper'),
]

def _add_dll_directories_for_import():
    """Add project DLL dirs to Windows loader so .pyd can find dependencies (FFmpeg etc.)."""
    if sys.platform != 'win32' or not hasattr(os, 'add_dll_directory'):
        return
    dirs = [
        os.path.join(_PROJECT_ROOT, 'vendor', 'ffmpeg', 'lib'),
        os.path.join(_OUT_RELEASE, 'clientCorePythonWrapper'),
        os.path.join(_OUT_DEBUG, 'clientCorePythonWrapper'),
    ]
    for d in dirs:
        if os.path.isdir(d):
            os.add_dll_directory(d)


# Try to import callsClientPy
_add_dll_directories_for_import()

callsClientPy = None
found_client_path = None
last_client_import_error = None
for path in possible_client_paths:
    abs_path = os.path.abspath(path)
    if os.path.exists(abs_path):
        if abs_path not in sys.path:
            sys.path.insert(0, abs_path)
        try:
            import callsClientPy
            found_client_path = abs_path
            break
        except ImportError as e:
            last_client_import_error = e
            if abs_path in sys.path:
                sys.path.remove(abs_path)
            continue

if callsClientPy is None:
    tried_paths_str = "\n".join(f"  - {os.path.abspath(p)} {'(exists)' if os.path.exists(os.path.abspath(p)) else '(not found)'}"
                               for p in possible_client_paths)
    err_detail = f"\n\nLast import error: {last_client_import_error}" if last_client_import_error else ""
    raise ImportError(
        f"Could not find callsClientPy module.\n\n"
        f"Tried paths:\n{tried_paths_str}\n\n"
        f"Please build the project first. The module should be a .pyd file (Windows) "
        f"or .so file (Linux) in one of the above directories.{err_detail}"
    )

# Try to import callsServerPy
callsServerPy = None
found_server_path = None
last_server_import_error = None
for path in possible_server_paths:
    abs_path = os.path.abspath(path)
    if os.path.exists(abs_path):
        if abs_path not in sys.path:
            sys.path.insert(0, abs_path)
        try:
            import callsServerPy
            found_server_path = abs_path
            break
        except ImportError as e:
            last_server_import_error = e
            if abs_path in sys.path:
                sys.path.remove(abs_path)
            continue

if callsServerPy is None:
    tried_paths_str = "\n".join(f"  - {os.path.abspath(p)} {'(exists)' if os.path.exists(os.path.abspath(p)) else '(not found)'}"
                               for p in possible_server_paths)
    err_detail = f"\n\nLast import error: {last_server_import_error}" if last_server_import_error else ""
    raise ImportError(
        f"Could not find callsServerPy module.\n\n"
        f"Tried paths:\n{tried_paths_str}\n\n"
        f"Please build the project first. The module should be a .pyd file (Windows) "
        f"or .so file (Linux) in one of the above directories.{err_detail}"
    )


class CallbacksHandler(callsClientPy.EventListener):
    """Thread-safe handler for collecting events from client callbacks"""
    
    def __init__(self, name=""):
        super().__init__()
        self.name = name
        self.events = multiprocessing.Manager().list()
        self.event_condition = multiprocessing.Condition()
    
    def onAuthorizationResult(self, ec_value):
        # ec_value is now int (error code value)
        is_success = (ec_value == 0)
        status = "OK" if is_success else "ERROR"
        event = f"auth_result_{status}"
        self.events.append(event)
        print(f"[{self.name}] Authorization result: {status}")
    
    def onOutgoingCallTimeout(self, ec_value):
        self.events.append("max_time_reached")
        print(f"[{self.name}] Outgoing call timeout")
    
    def onOutgoingCallAccepted(self, nickname=""):
        self.events.append("outgoing_call_accepted")
        print(f"[{self.name}] Outgoing call accepted" + (f" (nickname: {nickname})" if nickname else ""))
    
    def onOutgoingCallDeclined(self):
        self.events.append("outgoing_call_declined")
        print(f"[{self.name}] Outgoing call declined")
    
    def onIncomingCall(self, friend_nickname):
        event = f"incoming_call_{friend_nickname}"
        self.events.append(event)
        print(f"[{self.name}] Incoming call from {friend_nickname}")
    
    def onIncomingCallExpired(self, ec_value, friend_nickname):
        event = f"call_expired_{friend_nickname}"
        self.events.append(event)
        print(f"[{self.name}] Incoming call expired from {friend_nickname}")
    
    def onConnectionDown(self):
        self.events.append("network_error")
        print(f"[{self.name}] Connection down")
    
    def onConnectionRestored(self):
        self.events.append("connection_restored")
        print(f"[{self.name}] Connection restored")
    
    def onCallEndedByRemote(self, ec_value):
        self.events.append("remote_ended_call")
        print(f"[{self.name}] Remote user ended call")
    
    def onIncomingScreenSharingStarted(self):
        self.events.append("incoming_screen_sharing_started")
        print(f"[{self.name}] Incoming screen sharing started")
    
    def onIncomingScreenSharingStopped(self):
        self.events.append("incoming_screen_sharing_stopped")
        print(f"[{self.name}] Incoming screen sharing stopped")
    
    def onIncomingCameraSharingStarted(self):
        self.events.append("incoming_camera_sharing_started")
        print(f"[{self.name}] Incoming camera sharing started")
    
    def onIncomingCameraSharingStopped(self):
        self.events.append("incoming_camera_sharing_stopped")
        print(f"[{self.name}] Incoming camera sharing stopped")
    
    def wait_for_event(self, event_name, timeout):
        """Wait for specific event with timeout"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if event_name in self.events:
                return True
            time.sleep(0.1)
        return False


def run_client(host, port, nickname, events_list, handler_name, test_scenario):
    """
    Legacy function for backward compatibility with hardcoded scenarios.
    Consider using run_client_flexible for new tests.
    """
    
    if test_scenario == "caller":
        scenario_func = lambda client, handler: _scenario_caller(client, handler, "responder_user")
    elif test_scenario == "responder_accept":
        scenario_func = lambda client, handler: _scenario_responder_accept(client, handler, "caller_user")
    elif test_scenario == "responder_decline":
        scenario_func = lambda client, handler: _scenario_responder_decline(client, handler, "caller_user")
    elif test_scenario == "audio_test":
        scenario_func = _scenario_audio_test
    else:
        scenario_func = None
    
    return run_client_flexible(host, port, nickname, events_list, handler_name, scenario_func)


def generate_unique_nickname(base_name):
    """Generate unique nickname by appending random suffix"""
    # Use timestamp + random to ensure uniqueness
    suffix = f"{int(time.time() * 1000) % 100000}_{random.randint(1000, 9999)}"
    return f"{base_name}_{suffix}"


def run_client_flexible(host, port, nickname, events_list, handler_name, scenario_func=None, wait_time=3):
    """
    Flexible client runner that accepts custom scenario function.
    
    Args:
        host: Server host address
        port: Server port
        nickname: User nickname for authorization
        events_list: Shared list for events (multiprocessing.Manager().list())
        handler_name: Name for the handler (for logging)
        scenario_func: Optional function(client, handler) that implements test scenario
        wait_time: Time to wait before stopping client (default: 3 seconds)
    
    Returns:
        bool: True if client ran successfully, False otherwise
    
    Example:
        def custom_scenario(client, handler):
            client.start_outgoing_call("friend")
            handler.wait_for_event("call_result_OK", 10)
        
        events = multiprocessing.Manager().list()
        run_client_flexible("localhost", "8081", "user1", events, "user1", custom_scenario)
    """
    
    # Create handler inside this process (can't pickle C++ objects)
    process_handler = CallbacksHandler(handler_name)
    process_handler.events = events_list  # Use shared list
    
    # Create Client instance
    client = callsClientPy.Client()
    
    # Initialize client with event listener
    if not client.init(host, port, process_handler):
        print(f"[{handler_name}] Failed to initialize client")
        return False
    
    time.sleep(1)
    
    # Authorize
    ec_value = client.authorize(nickname)  # Returns int now
    if not process_handler.wait_for_event("auth_result_OK", 5):
        print(f"[{handler_name}] Authorization failed or timeout")
        client.stop()
        return False
    
    # Run scenario if provided
    if scenario_func:
        scenario_func(client, process_handler)
    
    time.sleep(wait_time)
    
    # Stop client
    client.stop()
    return True


def _scenario_caller(client, handler, target_user):
    """Scenario: call specific user"""
    print(f"[{handler.name}] Starting call to {target_user}")
    client.start_outgoing_call(target_user)
    handler.wait_for_event("call_result_OK", 10)


def _scenario_responder_accept(client, handler, caller_user):
    """Scenario: wait for call from specific user and accept"""
    print(f"[{handler.name}] Waiting for incoming call...")
    if handler.wait_for_event(f"incoming_call_{caller_user}", 10):
        print(f"[{handler.name}] Accepting call")
        client.accept_call(caller_user)
        handler.wait_for_event("accept_result_OK", 5)


def _scenario_responder_decline(client, handler, caller_user):
    """Scenario: wait for call from specific user and decline"""
    print(f"[{handler.name}] Waiting for incoming call...")
    if handler.wait_for_event(f"incoming_call_{caller_user}", 10):
        print(f"[{handler.name}] Declining call")
        client.decline_call(caller_user)


def _scenario_audio_test(client, handler):
    """Scenario: test audio controls"""
    print(f"[{handler.name}] Testing audio controls")
    client.mute_microphone(True)
    client.mute_microphone(False)
    client.set_input_volume(80)
    client.set_output_volume(90)


def run_server_in_process(port, stop_event):
    """Runs server in a separate process"""
    try:
        server = callsServerPy.Server(port)
        
        def server_thread():
            server.run()
        
        server_thread = threading.Thread(target=server_thread)
        server_thread.daemon = True
        server_thread.start()
        
        while not stop_event.is_set():
            time.sleep(0.1)
        
        server.stop()
        print("Server stopped")
        
    except Exception as e:
        print(f"Server error: {e}")


class TestRunner:
    def __init__(self, port="8081"):
        self.port = port
        self.server_process = None
        self.stop_event = None
    
    def start_server(self):
        """Starts server using callsServerPy module"""
        try:
            self.stop_event = multiprocessing.Event()
            
            self.server_process = multiprocessing.Process(
                target=run_server_in_process,
                args=(self.port, self.stop_event)
            )
            
            self.server_process.start()
            time.sleep(3)
            print(f"Server started on port {self.port}")
            return True
            
        except Exception as e:
            print(f"Failed to start server: {e}")
            return False
    
    def stop_server(self):
        """Stops server"""
        if self.stop_event:
            self.stop_event.set()
        
        if self.server_process and self.server_process.is_alive():
            self.server_process.join(timeout=5)
            if self.server_process.is_alive():
                self.server_process.terminate()
    
    def run_test(self, test_func, test_name):
        print(f"\n{'='*50}")
        print(f"Running test: {test_name}")
        print(f"{'='*50}")
        try:
            result = test_func()
            if result:
                print(f"✅ Test {test_name}: PASSED")
            else:
                print(f"❌ Test {test_name}: FAILED")
            return result
        except Exception as e:
            print(f"❌ Test {test_name}: FAILED - {e}")
            import traceback
            traceback.print_exc()
            return False
