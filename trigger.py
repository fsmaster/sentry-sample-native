"""HTTP wrapper for the playground dashboard: POST /trigger runs ./sample."""
import json
import os
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer


class Trigger(BaseHTTPRequestHandler):
    def do_POST(self):
        body = json.loads(self.rfile.read(int(self.headers.get("content-length", 0))) or b"{}")
        kind = "crash" if body.get("kind", "crash") == "crash" else "error"
        run = str(body.get("run", "manual"))[:40]
        fresh = "1" if body.get("fresh", True) else "0"
        env = dict(os.environ)
        if body.get("dsn"):  # dashboard "Use my DSN"
            env["SENTRY_DSN"] = body["dsn"]
        p = subprocess.run(["./sample", kind, run, fresh], capture_output=True, text=True, timeout=30, env=env)
        reply = {"exit_code": p.returncode, "output": p.stdout.strip()}
        if "event id:" in p.stdout:
            reply["event_id"] = p.stdout.split("event id:")[1].strip()
        data = json.dumps(reply).encode()
        self.send_response(200)
        self.send_header("content-type", "application/json")
        self.end_headers()
        self.wfile.write(data)


HTTPServer(("", 8080), Trigger).serve_forever()
