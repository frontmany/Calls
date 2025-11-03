import time
import threading
import multiprocessing
import sys

sys.path.append('C:/prj/Callifornia/build/clientPy/Release')
import callsClientPy

sys.path.append('C:/prj/Callifornia/build/serverPy/Release')
import callsServerPy


class CallbacksHandler:
    """Thread-safe handler for collecting events from client callbacks"""
    
    def __init__(self, name=""):
        self.name = name
        self.events = multiprocessing.Manager().list()
        self.event_condition = multiprocessing.Condition()
    
    def onAuthorizationResult(self, ec):
        event = f"auth_result_{ec.name}"
        self.events.append(event)
        print(f"[{self.name}] Authorization result: {ec.name}")
    
    def onStartCallingResult(self, ec):
        event = f"call_result_{ec.name}"
        self.events.append(event)
        print(f"[{self.name}] Start calling result: {ec.name}")
    
    def onAcceptCallResult(self, ec, nickname):
        event = f"accept_result_{ec.name}"
        self.events.append(event)
        print(f"[{self.name}] Accept call result: {ec.name} from {nickname}")
    
    def onMaximumCallingTimeReached(self):
        self.events.append("max_time_reached")
        print(f"[{self.name}] Maximum calling time reached")
    
    def onCallingAccepted(self):
        self.events.append("calling_accepted")
        print(f"[{self.name}] Calling accepted")
    
    def onCallingDeclined(self):
        self.events.append("calling_declined")
        print(f"[{self.name}] Calling declined")
    
    def onIncomingCall(self, friend_nickname):
        event = f"incoming_call_{friend_nickname}"
        self.events.append(event)
        print(f"[{self.name}] Incoming call from {friend_nickname}")
    
    def onIncomingCallExpired(self, friend_nickname):
        event = f"call_expired_{friend_nickname}"
        self.events.append(event)
        print(f"[{self.name}] Incoming call expired from {friend_nickname}")
    
    def onNetworkError(self):
        self.events.append("network_error")
        print(f"[{self.name}] Network error")
    
    def onConnectionRestored(self):
        self.events.append("connection_restored")
        print(f"[{self.name}] Connection restored")
    
    def onRemoteUserEndedCall(self):
        self.events.append("remote_ended_call")
        print(f"[{self.name}] Remote user ended call")
    
    def wait_for_event(self, event_name, timeout):
        """Wait for specific event with timeout"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if event_name in self.events:
                return True
            time.sleep(0.1)
        return False


def run_client(host, port, nickname, process_handler, test_scenario):
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
    
    return run_client_flexible(host, port, nickname, process_handler, scenario_func)


def run_client_flexible(host, port, nickname, process_handler, scenario_func=None, wait_time=3):
    """
    Flexible client runner that accepts custom scenario function.
    
    Args:
        host: Server host address
        port: Server port
        nickname: User nickname for authorization
        process_handler: Handler object for callbacks
        scenario_func: Optional function(callsClientPy, handler) that implements test scenario
        wait_time: Time to wait before stopping client (default: 3 seconds)
    
    Returns:
        bool: True if client ran successfully, False otherwise
    
    Example:
        def custom_scenario(client, handler):
            client.start_calling("friend")
            handler.wait_for_event("call_result_OK", 10)
        
        run_client_flexible("localhost", "8081", "user1", handler, custom_scenario)
    """
    
    class LocalHandler(callsClientPy.Handler):
        def __init__(self, ph):
            super().__init__()
            self.process_handler = ph
        
        def onAuthorizationResult(self, ec):
            self.process_handler.onAuthorizationResult(ec)
        
        def onStartCallingResult(self, ec):
            self.process_handler.onStartCallingResult(ec)
        
        def onAcceptCallResult(self, ec, nickname):
            self.process_handler.onAcceptCallResult(ec, nickname)
        
        def onMaximumCallingTimeReached(self):
            self.process_handler.onMaximumCallingTimeReached()
        
        def onCallingAccepted(self):
            self.process_handler.onCallingAccepted()
        
        def onCallingDeclined(self):
            self.process_handler.onCallingDeclined()
        
        def onIncomingCall(self, friend_nickname):
            self.process_handler.onIncomingCall(friend_nickname)
        
        def onIncomingCallExpired(self, friend_nickname):
            self.process_handler.onIncomingCallExpired(friend_nickname)
        
        def onNetworkError(self):
            self.process_handler.onNetworkError()
        
        def onConnectionRestored(self):
            self.process_handler.onConnectionRestored()
        
        def onRemoteUserEndedCall(self):
            self.process_handler.onRemoteUserEndedCall()
    
    handler = LocalHandler(process_handler)
    
    if not callsClientPy.init(host, port, handler):
        return False
    
    callsClientPy.run()
    time.sleep(1)
    
    callsClientPy.authorize(nickname)
    if not process_handler.wait_for_event("auth_result_OK", 5):
        callsClientPy.stop()
        return False
    
    if scenario_func:
        scenario_func(callsClientPy, process_handler)
    
    time.sleep(wait_time)
    callsClientPy.stop()
    return True


def _scenario_caller(client, handler, target_user):
    """Scenario: call specific user"""
    print(f"[{handler.name}] Starting call to {target_user}")
    client.start_calling(target_user)
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
        server = callsServerPy.CallsServer(port)
        
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
            return False