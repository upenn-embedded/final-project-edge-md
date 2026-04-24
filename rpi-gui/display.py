import sys
import json
import socket
import threading
import customtkinter as ctk

# Configuration
HOST = 'localhost'
PORT = 5001
FULLSCREEN = sys.platform != "darwin"  # Run fullscreen on non-Mac devices (like Raspberry Pi)

class TranslationDisplayApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        # Setup standard CTk theme
        ctk.set_appearance_mode("Light")
        ctk.set_default_color_theme("blue")

        self.title("Translation Display")
        
        if FULLSCREEN:
            self.attributes('-fullscreen', True)
            # Geometry fallback if fullscreen fails or doesn't lock resolution
            self.geometry("800x480") 
        else:
            self.geometry("800x480")
            self.resizable(False, False)

        # Configure Grid Layout
        self.grid_rowconfigure(1, weight=1)
        self.grid_columnconfigure(0, weight=1)

        # Fonts - Large and readable for a 5" screen
        self.title_font = ctk.CTkFont(family="Helvetica", size=32, weight="bold")
        self.text_font = ctk.CTkFont(family="Helvetica", size=42)
        self.status_font = ctk.CTkFont(family="Helvetica", size=24, slant="italic")

        # Set main window background to soft gray
        self.configure(fg_color="#F2F2F7")

        # --- Top Bar ---
        self.top_frame = ctk.CTkFrame(self, fg_color="#FFFFFF", corner_radius=0)
        self.top_frame.grid(row=0, column=0, sticky="ew")
        
        self.direction_label = ctk.CTkLabel(
            self.top_frame, 
            text="Waiting for translation...", 
            font=self.title_font,
            text_color="#1C1C1E"
        )
        self.direction_label.pack(pady=15)

        # --- Main Area ---
        self.main_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.main_frame.grid(row=1, column=0, sticky="nsew", padx=20, pady=20)
        self.main_frame.grid_columnconfigure(0, weight=1)
        self.main_frame.grid_rowconfigure((0, 1), weight=1)

        # Top Panel (Original Data)
        self.left_panel = ctk.CTkTextbox(
            self.main_frame, 
            font=self.text_font, 
            wrap="word",
            fg_color="#FFFFFF",
            text_color="#8E8E93",
            corner_radius=15,
            border_width=1,
            border_color="#E5E5EA"
        )
        self.left_panel.grid(row=0, column=0, sticky="nsew", pady=(0, 10))
        self.left_panel.insert("1.0", "Original text will appear here.")
        self.left_panel.configure(state="disabled")

        # Bottom Panel (Translated Data)
        self.right_panel = ctk.CTkTextbox(
            self.main_frame, 
            font=self.text_font, 
            wrap="word",
            fg_color="#FFFFFF",
            text_color="#000000",
            corner_radius=15,
            border_width=1,
            border_color="#E5E5EA"
        )
        self.right_panel.grid(row=1, column=0, sticky="nsew", pady=(10, 0))
        self.right_panel.insert("1.0", "Translated text will appear here.")
        self.right_panel.configure(state="disabled")

        # --- Bottom Bar ---
        self.bottom_frame = ctk.CTkFrame(self, fg_color="#FFFFFF", corner_radius=0)
        self.bottom_frame.grid(row=2, column=0, sticky="ew")
        
        self.status_label = ctk.CTkLabel(
            self.bottom_frame, 
            text="Status: Waiting for connection...", 
            font=self.status_font,
            text_color="#DD0000"
        )
        self.status_label.pack(side="left", padx=20, pady=10)

        # Start TCP Socket Thread
        self.running = True
        self.server_thread = threading.Thread(target=self.socket_server_loop, daemon=True)
        self.server_thread.start()

    def update_ui(self, payload):
        """Called on main thread via .after() to update widgets instantly."""
        # Update top bar direction indicator
        direction = payload.get("direction", "")
        
        # Ensure full language names are displayed
        direction = direction.replace("en", "English").replace("es", "Spanish").replace("→", " to ").replace("->", " to ")
        
        if direction:
            self.direction_label.configure(text=direction)
        
        # Extract text components
        original = payload.get("original", "")
        translated = payload.get("translated", "")

        # Update left panel (source)
        self.left_panel.configure(state="normal")
        self.left_panel.delete("1.0", "end")
        self.left_panel.insert("1.0", original)
        self.left_panel.configure(state="disabled")

        # Update right panel (target)
        self.right_panel.configure(state="normal")
        self.right_panel.delete("1.0", "end")
        self.right_panel.insert("1.0", translated)
        self.right_panel.configure(state="disabled")

    def update_status(self, connected):
        """Called on main thread via .after() to switch connection states."""
        if connected:
            self.status_label.configure(text="Status: Connected", text_color="#008800")
        else:
            self.status_label.configure(text="Status: Waiting for connection...", text_color="#DD0000")

    def socket_server_loop(self):
        """Thread background loop listening for JSON socket connections."""
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # Avoid 'Address already in use' errors
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            server_socket.bind((HOST, PORT))
            server_socket.listen(1)
        except Exception as e:
            print(f"Error binding to socket: {e}")
            return

        while self.running:
            self.after(0, self.update_status, False)
            try:
                # Set a timeout so we can exit the loop if the app is closed
                server_socket.settimeout(1.0)
                conn, addr = server_socket.accept()
                
                # We got a connection from the external script
                self.after(0, self.update_status, True)
                
                with conn:
                    # Stream over file-like interface, expecting newline-delimited JSON payloads
                    f = conn.makefile('r', encoding='utf-8')
                    while self.running:
                        line = f.readline()
                        if not line:
                            break # Empty string means disconnect
                        line = line.strip()
                        if not line:
                            continue # Ignore empty strings
                        try:
                            payload = json.loads(line)
                            # Pass payload to the main UI thread seamlessly
                            self.after(0, self.update_ui, payload)
                        except json.JSONDecodeError:
                            print(f"Invalid JSON received: {line}")
            except socket.timeout:
                # Normal behavior on waiting, allows rechecking self.running
                continue
            except Exception as e:
                # Print other network exceptions only if app is still live
                if self.running:
                    print(f"Socket connection error: {e}")
                    
        server_socket.close()

    def destroy(self):
        """Intercept closure to ensure graceful thread exit."""
        self.running = False
        super().destroy()

if __name__ == "__main__":
    app = TranslationDisplayApp()
    app.mainloop()
