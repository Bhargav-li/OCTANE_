import importlib
import threading
import time
import RPi.GPIO as GPIO

# ─────────────────────────────────────────────
#  CONFIG — 3 GPIO pins (BCM numbering)
# ─────────────────────────────────────────────
PIN_A = 17    # LSB (least significant bit)
PIN_B = 27    # middle bit
PIN_C = 22    # MSB (most significant bit)

POLL_INTERVAL = 0.1   # seconds between pin checks

# ─────────────────────────────────────────────
#  BINARY MAP
#  Key   : (PIN_C, PIN_B, PIN_A)  → binary string
#  Value : script name or None
#
#  000 → None     (do nothing)
#  001 → script1
#  010 → script2
#  011 → script3
#  100 → script4
#  101 → script5
#  110 → script6
#  111 → None     (do nothing)
# ─────────────────────────────────────────────
SCRIPT_MAP = {
    (0, 0, 0): None,        # 000 — do nothing
    (0, 0, 1): "script1",   # 001
    (0, 1, 0): "script2",   # 010
    (0, 1, 1): "script3",   # 011
    (1, 0, 0): "script4",   # 100
    (1, 0, 1): "script5",   # 101
    (1, 1, 0): "script6",   # 110
    (1, 1, 1): None,        # 111 — do nothing
}

# ─────────────────────────────────────────────
#  Global state
# ─────────────────────────────────────────────
stop_event     = threading.Event()
current_thread = None
current_combo  = None   # last pin combination, to avoid re-triggering

# ─────────────────────────────────────────────
#  Core function: stop current, start new
# ─────────────────────────────────────────────
def run_script(script_name):
    global current_thread, stop_event

    # Stop currently running script
    if current_thread and current_thread.is_alive():
        print(f"\n⏹  Stopping {current_thread.name}...")
        stop_event.set()
        current_thread.join(timeout=5)

        if current_thread.is_alive():
            print("⚠️  Warning: script did not stop in time!")
        stop_event.clear()
        print()

    # Start new script
    print(f"▶  Starting {script_name}...\n")
    module = importlib.import_module(script_name)
    current_thread = threading.Thread(
        target=module.run,
        args=(stop_event,),
        name=script_name,
        daemon=True
    )
    current_thread.start()

def stop_current():
    global current_thread, stop_event

    if current_thread and current_thread.is_alive():
        print(f"\n⏹  Stopping {current_thread.name} (000 or 111 — do nothing)...")
        stop_event.set()
        current_thread.join(timeout=5)
        stop_event.clear()
        print("✅  Stopped. Idle.\n")

# ─────────────────────────────────────────────
#  Read all 3 pins and return combo tuple
# ─────────────────────────────────────────────
def read_pins():
    a = GPIO.input(PIN_A)
    b = GPIO.input(PIN_B)
    c = GPIO.input(PIN_C)
    return (c, b, a)   # MSB → LSB order matches binary string

# ─────────────────────────────────────────────
#  GPIO pin listener — runs in its own thread
# ─────────────────────────────────────────────
def listen_to_pins():
    global current_combo

    print(f"👂 Monitoring pins — A:{PIN_A}, B:{PIN_B}, C:{PIN_C} (BCM)\n")

    while True:
        combo = read_pins()

        # Only act if combination has changed
        if combo != current_combo:
            current_combo = combo

            binary_str  = "".join(str(b) for b in combo)
            script_name = SCRIPT_MAP.get(combo)

            print(f"\n📍 Pins → {binary_str}  (C={combo[0]} B={combo[1]} A={combo[2]})")

            if script_name:
                run_script(script_name)
            else:
                print(f"⏸  {binary_str} = do nothing")
                stop_current()

        time.sleep(POLL_INTERVAL)

# ─────────────────────────────────────────────
#  Main
# ─────────────────────────────────────────────
def main():
    print("=" * 48)
    print("   MOTHER FILE — 3-Pin Binary Script Controller")
    print("=" * 48)
    print(f"  Pins     : A={PIN_A} (LSB)  B={PIN_B}  C={PIN_C} (MSB)")
    print()
    print("  Binary │ Script")
    print("  ───────┼────────────")
    for combo, name in SCRIPT_MAP.items():
        binary_str = "".join(str(b) for b in combo)
        label = name if name else "— do nothing —"
        print(f"   {binary_str}   │ {label}")
    print("=" * 48)

    # GPIO setup
    GPIO.setmode(GPIO.BCM)
    for pin in [PIN_A, PIN_B, PIN_C]:
        GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

    # Start listener thread
    listener = threading.Thread(target=listen_to_pins, daemon=True)
    listener.start()

    # Keep main thread alive
    try:
        while True:
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n\n⏹  Ctrl+C — shutting down...")
        stop_event.set()
        if current_thread:
            current_thread.join()
        GPIO.cleanup()
        print("👋 Goodbye!")

if __name__ == "__main__":
    main()
