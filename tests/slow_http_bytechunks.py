import http.server
import socketserver
import time
import sys
import os
from threading import Thread

PORT = 8000
CHUNK_SIZE = 80  # 80 bytes per chunk
CHUNKS_PER_SECOND = 10

class ChunkedContentHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/content':
            if not hasattr(self.server, 'file_path'):
                self.send_error(500, "No file specified")
                return
            
            try:
                with open(self.server.file_path, 'rb') as file:  # Read in binary mode
                    self.send_response(200)
                    self.send_header('Content-type', 'application/octet-stream')
                    self.end_headers()
                    
                    # Stream file in chunks with delay
                    while True:
                        chunk = file.read(CHUNK_SIZE)
                        if not chunk:
                            break
                        self.wfile.write(chunk)
                        self.wfile.flush()
                        time.sleep(1 / CHUNKS_PER_SECOND)
            except FileNotFoundError:
                self.send_error(404, "File not found")
            except Exception as e:
                self.send_error(500, f"Server error: {str(e)}")
        else:
            self.send_error(404, "Not Found")

class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    """Handle requests in a separate thread."""

def run_server(file_path):
    if not os.path.isfile(file_path):
        print(f"Error: File '{file_path}' not found")
        return

    server = ThreadedHTTPServer(("", PORT), ChunkedContentHandler)
    server.file_path = file_path  # Store the file path in the server instance
    
    print(f"Serving content from '{file_path}' at http://localhost:{PORT}/content")
    print(f"Streaming in {CHUNK_SIZE}-byte chunks at {CHUNKS_PER_SECOND} chunks/sec")
    print(f"Effective transfer rate: {CHUNK_SIZE * CHUNKS_PER_SECOND} bytes/sec")
    print("Press Ctrl+C to stop the server")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.shutdown()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python chunked_server.py <filename>")
        sys.exit(1)
    
    run_server(sys.argv[1])