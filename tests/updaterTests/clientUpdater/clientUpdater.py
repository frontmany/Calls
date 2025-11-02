import sys
sys.path.append('C:/prj/Callifornia/build/clientUpdaterPy/Debug')
import clientUpdaterPy


class UpdateCallbacksHandler(clientUpdaterPy.CallbacksInterface):
    def __init__(self, manager):
        super().__init__()
        self.manager = manager

    def onUpdatesCheckResult(self, result):
        """Called when server responds to update check request"""
        print(f"Received update check result: {result}")
        
        if result == clientUpdaterPy.UpdatesCheckResult.UPDATE_NOT_NEEDED:
            print("✅ Your version is up to date!")
            
        elif result == clientUpdaterPy.UpdatesCheckResult.REQUIRED_UPDATE:
            print("🔴 MANDATORY update required!")
            print("Starting automatic update...")
            self.manager.start_update()
            
        elif result == clientUpdaterPy.UpdatesCheckResult.POSSIBLE_UPDATE:
            print("🟡 Optional update available")
            if self.manager.ask_user_for_update():
                self.manager.start_update()

    def onLoadingProgress(self, progress):
        """Called to report loading progress"""
        print(f"📥 Loading progress: {progress * 100:.1f}%")

    def onUpdateLoaded(self, emptyUpdate):
        """Called when update is fully loaded"""
        if emptyUpdate:
            print("✅ Update loaded (no files changed)")
        else:
            print("✅ Update successfully loaded!")
            print("Now you can install it or restart the application")
            self.manager.install_update()

    def onError(self):
        """Called on ANY error during update process"""
        print("❌ Error occurred during update!")
        print("Possible causes:")
        print("- No connection to server")
        print("- File download error")
        print("- Corrupted data")
        self.manager.show_error_message()


class UpdateManager:
    def __init__(self):
        self.current_version = "1.0.0"
        self.callbacks_handler = UpdateCallbacksHandler(self)
        clientUpdaterPy.init(self.callbacks_handler)

    def ask_user_for_update(self):
        """Ask user about installing update"""
        response = input("Install update? (y/n): ")
        return response.lower() == 'y'

    def start_update(self):
        """Start update download process"""
        print("📥 Starting update download...")
        os_type = clientUpdaterPy.OperationSystemType.WINDOWS
        clientUpdaterPy.startUpdate(os_type)

    def install_update(self):
        """Install downloaded update"""
        print("🔄 Installing update...")

    def show_error_message(self):
        """Show error message"""
        print("Please check your connection and try again")


def main():
    manager = UpdateManager()
    
    try:
        # 1. Connect to server
        if clientUpdaterPy.connect("192.168.1.44", "8081"):
            print("✅ Connected to update server")
        else:
            print("❌ Failed to connect to update server")
            return
        
        # 2. Check for updates
        print("🔍 Checking for updates...")
        clientUpdaterPy.checkUpdates(manager.current_version)
        
        # Wait for async events processing
        input("Press Enter to exit...")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        clientUpdaterPy.disconnect()
        print("Disconnected from server")


if __name__ == "__main__":
    main()