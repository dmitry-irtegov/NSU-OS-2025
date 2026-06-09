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
                with open(self.server.file_path, 'rb') as file:
                    self.send_response(200)
                    self.send_header('Content-type', 'application/octet-stream')
                    self.end_headers()
                    
                    # Stream file in chunks with delay
                    while True:
                        chunk = file.read(CHUNK_SIZE)
                        if not chunk:
                            break
                        self.wfile.write(chunk)
                        # Flush is needed in Python 3.5 to ensure chunks are sent immediately
                        self.wfile.flush()  
                        time.sleep(1.0 / CHUNKS_PER_SECOND)
            except IOError as e:
                self.send_error(404, "File not found")
            except Exception as e:
                self.send_error(500, "Server error: {}".format(str(e)))
        else:
            self.send_error(404, "Not Found")

# Python 3.5 compatible threaded server
class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True  # Ensure threads exit when main process exits

def run_server(file_path):
    if not os.path.isfile(file_path):
        print("Error: File '{}' not found".format(file_path))
        return

    server = ThreadedHTTPServer(("", PORT), ChunkedContentHandler)
    # Store file path in server instance (Python 3.5 compatible way)
    server.file_path = file_path
    
    print("Serving content from '{}' at http://localhost:{}/content".format(file_path, PORT))
    print("Streaming in {}-byte chunks at {} chunks/sec".format(CHUNK_SIZE, CHUNKS_PER_SECOND))
    print("Effective transfer rate: {} bytes/sec".format(CHUNK_SIZE * CHUNKS_PER_SECOND))
    print("Press Ctrl+C to stop the server")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.shutdown()
        server.server_close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python {} <filename>".format(sys.argv[0]))
        sys.exit(1)
    
    run_server(sys.argv[1])