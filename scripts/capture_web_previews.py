#!/usr/bin/env python3
"""Render the checked-out web UI, using its existing simulated browser bridge.

Development only: pip install playwright; use an installed Chromium. This never
loads the plugin, exercises DSP or establishes Mac/WebKit/Logic compatibility.
"""

import argparse
import json
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread

from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chromium", default="/usr/bin/chromium")
    parser.add_argument("--out", type=Path, default=Path("docs/previews-web"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    args.out.mkdir(parents=True, exist_ok=True)

    class QuietHandler(SimpleHTTPRequestHandler):
        def log_message(self, *_):
            pass

    server = ThreadingHTTPServer(
        ("127.0.0.1", 0), partial(QuietHandler, directory=str(root / "ui"))
    )
    Thread(target=server.serve_forever, daemon=True).start()
    errors, records = [], []
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(
                executable_path=args.chromium,
                headless=True,
                args=["--no-sandbox", "--disable-dev-shm-usage"],
            )
            page = browser.new_page(viewport={"width": 1440, "height": 900})
            page.on("pageerror", lambda error: errors.append(str(error)))
            page.goto(f"http://127.0.0.1:{server.server_port}")
            page.wait_for_function(
                "document.querySelector('#stageArt svg') && "
                "document.querySelector('#keys').children.length > 0"
            )
            assert page.evaluate("Rastro.simulated"), "Expected browser-only simulated bridge"
            # Let the existing simulated-bridge toast disappear so it covers no controls.
            page.wait_for_timeout(4500)

            def capture(name):
                # Capture the whole viewport, without altering the UI or hiding controls.
                page.screenshot(path=str(args.out / f"{name}.png"), full_page=True)
                broken = page.locator("img").evaluate_all(
                    "es=>es.filter(e=>e.complete && !e.naturalWidth).map(e=>e.src)"
                )
                assert not broken, f"Broken images: {broken}"
                screen = page.locator("body").get_attribute("data-screen")
                if screen in ("perform", "limpo"):
                    buttons = page.locator("#rackList .mod .pwr")
                    assert buttons.count() == 6
                    for button in buttons.all():
                        button.scroll_into_view_if_needed()
                        assert button.evaluate("""e=>{
                            const r=e.getBoundingClientRect();
                            return e.contains(document.elementFromPoint(r.x+r.width/2,r.y+r.height/2));
                        }"""), "A module's bypass button is covered"
                    page.locator("#rackList").evaluate("e=>e.scrollTop=0")
                    keys = page.locator("#keys").bounding_box()
                    assert keys and keys["y"] + keys["height"] <= page.viewport_size["height"] + 1
                    if screen == "limpo":
                        rack = page.locator("#rackPanel").bounding_box()
                        stage = page.locator("#stage").bounding_box()
                        assert rack["y"] >= stage["y"] + stage["height"]
                        assert rack["y"] + rack["height"] <= keys["y"], "Rack covers keyboard"
                if screen == "locker":
                    stage = page.locator("#stage").bounding_box()
                    character = page.locator("#charPos").bounding_box()
                    assert character and character["x"] >= stage["x"]
                    assert character["x"] + character["width"] <= stage["x"] + stage["width"], "Locker clips character"
                records.append({"file": f"{name}.png", "viewport": page.viewport_size,
                                "screen": screen})

            capture("pr5-studio-standard")
            page.set_viewport_size({"width": 1100, "height": 760})
            page.wait_for_timeout(350)
            capture("pr5-studio-compact")
            page.set_viewport_size({"width": 1440, "height": 900})
            for screen, name in [("limpo", "pr5-sample"), ("locker", "pr5-locker")]:
                page.locator("#menuBtn").click()
                page.locator(f'#menu [data-go="{screen}"]').click()
                page.wait_for_timeout(500)
                assert page.locator("body").get_attribute("data-screen") == screen
                capture(name)
                page.set_viewport_size({"width": 1100, "height": 760})
                page.wait_for_timeout(350)
                capture(name + "-compact")
                page.set_viewport_size({"width": 1440, "height": 900})
            browser.close()
    finally:
        server.shutdown()
        server.server_close()

    assert not errors, f"JavaScript errors: {errors}"
    report = {"platform": "Linux / Chromium", "simulated_bridge": True,
              "plugin_or_dsp_test": False, "page_errors": errors, "captures": records}
    (args.out / "capture-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
