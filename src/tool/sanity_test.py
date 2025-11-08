#!/usr/bin/env python3
"""
Sanity test script for Sword of Secrets firmware.
Tests basic USB-Serial interface and commands after firmware flashing.
"""

import serial
import time
import sys
import re

SERPORT = "/dev/ttyUSB0"
BAUDRATE = 115200
TIMEOUT = 2.0
INIT_TIMEOUT = 5.0

class SanityTest:
    def __init__(self, port=SERPORT, baudrate=BAUDRATE):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.buffer = ""
        self.test_results = []
        
    def connect(self):
        """Connect to the serial port."""
        try:
            self.ser = serial.Serial(
                self.port,
                baudrate=self.baudrate,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=TIMEOUT
            )
            print(f"✓ Connected to {self.port} at {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            print(f"✗ Failed to connect: {e}")
            return False
    
    def read_until_prompt(self, timeout=INIT_TIMEOUT):
        """Read from serial until we see the '>> ' prompt."""
        start_time = time.time()
        data = b""
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                chunk = self.ser.read(self.ser.in_waiting)
                data += chunk
                text = data.decode('utf-8', errors='ignore')
                
                # Check for prompt
                if '>> ' in text:
                    return text
            time.sleep(0.1)
        
        # Return what we got even if no prompt found
        return data.decode('utf-8', errors='ignore')
    
    def send_command(self, cmd, expect_prompt=True, timeout=TIMEOUT):
        """Send a command and read response."""
        if not self.ser or not self.ser.is_open:
            return None
            
        # Clear any pending data
        self.ser.reset_input_buffer()
        
        # Send command with newline
        cmd_bytes = (cmd + '\r\n').encode('utf-8')
        self.ser.write(cmd_bytes)
        
        # Read response
        start_time = time.time()
        response = b""
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                chunk = self.ser.read(self.ser.in_waiting)
                response += chunk
                
                # If we expect a prompt, wait for it
                if expect_prompt:
                    text = response.decode('utf-8', errors='ignore')
                    if '>> ' in text:
                        break
            time.sleep(0.05)
        
        return response.decode('utf-8', errors='ignore')
    
    def test_initialization(self):
        """Test that device initializes properly."""
        print("\n[Test 2] Checking device initialization...")
        
        # After REBOOT, read initialization messages
        init_text = self.read_until_prompt(timeout=INIT_TIMEOUT)
        
        # Check for expected messages
        checks = {
            'Flash initialization': 'Flash initialization success' in init_text or 'Flash initialization failed' in init_text,
            'Initializing message': 'Initializing' in init_text,
            'Prompt received': '>> ' in init_text
        }
        
        all_passed = all(checks.values())
        
        if all_passed:
            print("  ✓ Device initialized successfully")
            print(f"  ✓ Flash init message found")
            print(f"  ✓ Initialization message found")
            print(f"  ✓ Command prompt received")
        else:
            print("  ✗ Initialization check failed:")
            for check, passed in checks.items():
                status = "✓" if passed else "✗"
                print(f"    {status} {check}")
        
        if not all_passed:
            print(f"\n  Received text:\n{init_text[:500]}")
        
        self.test_results.append(("Initialization", all_passed))
        return all_passed
    
    def test_reset_command(self):
        """Test RESET command."""
        print("\n[Test 3] Testing RESET command...")
        
        response = self.send_command("RESET", timeout=3.0)
        
        # RESET should print "Resetting challenge..." and "Resetting quest..." and "Done."
        checks = {
            'Response received': response is not None and len(response) > 0,
            'Resetting messages': 'Resetting' in response or 'Done' in response,
            'Prompt after command': '>> ' in response
        }
        
        all_passed = all(checks.values())
        
        if all_passed:
            print("  ✓ RESET command executed successfully")
        else:
            print("  ✗ RESET command failed:")
            for check, passed in checks.items():
                status = "✓" if passed else "✗"
                print(f"    {status} {check}")
            if response:
                print(f"  Response: {response[:200]}")
        
        self.test_results.append(("RESET command", all_passed))
        return all_passed
    
    def test_begin_end_commands(self):
        """Test BEGIN and END SPI commands."""
        print("\n[Test 4] Testing BEGIN/END commands...")
        
        # Test BEGIN
        response1 = self.send_command("BEGIN")
        begin_ok = response1 is not None and '>> ' in response1
        
        # Test END
        response2 = self.send_command("END")
        end_ok = response2 is not None and '>> ' in response2
        
        all_passed = begin_ok and end_ok
        
        if all_passed:
            print("  ✓ BEGIN command executed")
            print("  ✓ END command executed")
        else:
            print("  ✗ BEGIN/END commands failed:")
            print(f"    {'✓' if begin_ok else '✗'} BEGIN")
            print(f"    {'✓' if end_ok else '✗'} END")
        
        self.test_results.append(("BEGIN/END commands", all_passed))
        return all_passed
    
    def test_assert_release_commands(self):
        """Test ASSERT and RELEASE flash CS commands."""
        print("\n[Test 5] Testing ASSERT/RELEASE commands...")
        
        # Test ASSERT
        response1 = self.send_command("ASSERT")
        assert_ok = response1 is not None and '>> ' in response1
        
        # Test RELEASE
        response2 = self.send_command("RELEASE")
        release_ok = response2 is not None and '>> ' in response2
        
        all_passed = assert_ok and release_ok
        
        if all_passed:
            print("  ✓ ASSERT command executed")
            print("  ✓ RELEASE command executed")
        else:
            print("  ✗ ASSERT/RELEASE commands failed:")
            print(f"    {'✓' if assert_ok else '✗'} ASSERT")
            print(f"    {'✓' if release_ok else '✗'} RELEASE")
        
        self.test_results.append(("ASSERT/RELEASE commands", all_passed))
        return all_passed
    
    def test_data_command(self):
        """Test DATA command with full SPI sequence: ASSERT->BEGIN->DATA->END->RELEASE."""
        print("\n[Test 6] Testing DATA command (full SPI sequence)...")
        
        # Step 1: ASSERT - assert flash CS
        response1 = self.send_command("ASSERT")
        assert_ok = response1 is not None and '>> ' in response1
        
        # Step 2: BEGIN - initialize SPI
        response2 = self.send_command("BEGIN")
        begin_ok = response2 is not None and '>> ' in response2
        
        # Step 3: DATA - send SPI read command (3 1 0 0 0 0 0 0 0 0 0 0 0)
        # This is a read command: 0x03 (read) + address bytes
        data_cmd = "DATA 3 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
        response3 = self.send_command(data_cmd, timeout=2.0)
        
        # Expected output: "ff ff ff ff 00 00 00 00 0e 05 13 07 36"
        # DATA command should return hex bytes
        data_ok = response3 is not None and len(response3) > 0
        hex_output_ok = bool(re.search(r'[0-9a-fA-F]{2}', response3))
        
        # Extract hex bytes from response (remove whitespace, newlines, etc.)
        response_lower = response3.lower()
        # Look for the expected pattern of bytes
        expected_pattern = 'ff ff ff ff 00 00 00 00 0e 05 13 07'
        expected_found = expected_pattern in response_lower or all(
            byte in response_lower for byte in ['ff', '00', '0e', '05', '13', '07', '36']
        )
        
        # Step 4: END - end SPI
        response4 = self.send_command("END")
        end_ok = response4 is not None and '>> ' in response4
        
        # Step 5: RELEASE - release flash CS
        response5 = self.send_command("RELEASE")
        release_ok = response5 is not None and '>> ' in response5
        
        # Verify all steps
        checks = {
            'ASSERT command': assert_ok,
            'BEGIN command': begin_ok,
            'DATA response received': data_ok,
            'Hex output present': hex_output_ok,
            'Expected bytes found': expected_found,
            'END command': end_ok,
            'RELEASE command': release_ok
        }
        
        all_passed = all(checks.values())
        
        if all_passed:
            print("  ✓ All SPI sequence commands executed")
            print(f"  ✓ DATA response: {response3.strip()[:100]}")
        else:
            print("  ✗ DATA command sequence failed:")
            for check, passed in checks.items():
                status = "✓" if passed else "✗"
                print(f"    {status} {check}")
            if response3:
                print(f"  DATA Response: {response3[:200]}")
        
        self.test_results.append(("DATA command", all_passed))
        return all_passed
    
    def test_echo_behavior(self):
        """Test that device echoes characters."""
        print("\n[Test 7] Testing character echo...")
        
        # Send a command character by character to test echo
        test_cmd = "BEGIN"
        self.ser.reset_input_buffer()
        
        echoed = ""
        for char in test_cmd:
            self.ser.write(char.encode('utf-8'))
            time.sleep(0.05)
            if self.ser.in_waiting > 0:
                echoed += self.ser.read(1).decode('utf-8', errors='ignore')
        
        # Send newline
        self.ser.write(b'\r\n')
        time.sleep(0.2)
        
        # Read the rest
        if self.ser.in_waiting > 0:
            echoed += self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
        
        echo_ok = test_cmd in echoed or '>> ' in echoed
        
        if echo_ok:
            print("  ✓ Character echo working")
        else:
            print("  ✗ Character echo not working as expected")
            print(f"    Expected to see '{test_cmd}', got: {echoed[:100]}")
        
        self.test_results.append(("Character echo", echo_ok))
        return echo_ok
    
    def test_reboot_command(self):
        """Test REBOOT command (will reset the device)."""
        print("\n[Test 7] Testing REBOOT command...")
        print("  Note: This will reset the device and wait for it to come back up")
        
        # Send REBOOT command
        response = self.send_command("REBOOT", expect_prompt=False, timeout=1.0)
        
        # Check if we got the reboot message
        reboot_msg_ok = response is not None and 'Rebooting' in response
        
        if reboot_msg_ok:
            print("  ✓ REBOOT command sent, device resetting...")
        else:
            print("  ✗ REBOOT command may not have been sent properly")
            if response:
                print(f"  Response: {response[:200]}")
        
        # Wait for device to reset and come back up
        print("  Waiting for device to reboot...")
        time.sleep(2.0)  # Give device time to reset
        
        # Try to reconnect and wait for initialization
        init_text = self.read_until_prompt(timeout=INIT_TIMEOUT)
        
        # Check that device reinitialized
        checks = {
            'Reboot message received': reboot_msg_ok,
            'Device reinitialized': 'Flash initialization' in init_text or 'Initializing' in init_text,
            'Prompt after reboot': '>> ' in init_text
        }
        
        all_passed = all(checks.values())
        
        if all_passed:
            print("  ✓ Device rebooted successfully")
            print("  ✓ Device reinitialized and ready")
        else:
            print("  ✗ REBOOT test failed:")
            for check, passed in checks.items():
                status = "✓" if passed else "✗"
                print(f"    {status} {check}")
        
        self.test_results.append(("REBOOT command", all_passed))
        return all_passed
    
    def reboot_device(self, max_retries=3):
        """Reboot the device to ensure clean state."""
        print("\n[Test 1] Rebooting device for clean state...")
        
        reboot_msg_ok = False
        last_response = None
        
        # Retry mechanism for REBOOT command
        for attempt in range(1, max_retries + 1):
            if attempt > 1:
                print(f"  Retry attempt {attempt}/{max_retries}...")
                time.sleep(0.5)  # Brief delay between retries
                self.ser.reset_input_buffer()  # Clear any pending data
            
            # Send REBOOT command
            response = self.send_command("REBOOT", expect_prompt=False, timeout=1.0)
            last_response = response
            
            # Check if we got the reboot message
            if response and 'Rebooting' in response:
                reboot_msg_ok = True
                print("  ✓ REBOOT command sent, device resetting...")
                break
            else:
                if attempt < max_retries:
                    print(f"  ⚠ REBOOT command not acknowledged (attempt {attempt}), retrying...")
                    if response:
                        print(f"    Response: {response[:100]}")
                else:
                    print("  ⚠ REBOOT command may not have been sent properly")
                    if response:
                        print(f"  Response: {response[:200]}")
        
        # Wait for device to reset and come back up
        print("  Waiting for device to reboot...")
        time.sleep(3.0)  # Give device time to reset
        
        # Don't read initialization messages here - let INIT test do that
        # Just verify the reboot command was sent successfully
        checks = {
            'Reboot message received': reboot_msg_ok
        }
        
        all_passed = all(checks.values())
        
        if all_passed:
            print("  ✓ REBOOT command executed successfully")
        else:
            print("  ⚠ REBOOT may have issues:")
            for check, passed in checks.items():
                status = "✓" if passed else "✗"
                print(f"    {status} {check}")
            if last_response:
                print(f"  Last response: {last_response[:200]}")
        
        self.test_results.append(("REBOOT command", all_passed))
        return all_passed
    
    def run_all_tests(self, include_reboot=False):
        """Run all sanity tests."""
        print("=" * 70)
        print("Sword of Secrets - Firmware Sanity Test")
        print("=" * 70)
        
        if not self.connect():
            return False
        
        # Clear any initial data and wait a moment
        time.sleep(0.5)
        self.ser.reset_input_buffer()
        
        # Run tests - REBOOT first, then INIT, then RESET
        self.reboot_device()
        self.test_initialization()
        self.test_reset_command()
        self.test_begin_end_commands()
        self.test_assert_release_commands()
        self.test_data_command()
        self.test_echo_behavior()
        
        # Optional REBOOT test (resets the device)
        if include_reboot:
            self.test_reboot_command()
        
        # Print summary
        print("\n" + "=" * 70)
        print("Test Summary")
        print("=" * 70)
        
        passed = sum(1 for _, result in self.test_results if result)
        total = len(self.test_results)
        
        for test_name, result in self.test_results:
            status = "PASS" if result else "FAIL"
            print(f"  {status}: {test_name}")
        
        print(f"\nTotal: {passed}/{total} tests passed")
        
        if passed == total:
            print("\n✓ All sanity tests PASSED!")
            return True
        else:
            print(f"\n✗ {total - passed} test(s) FAILED")
            return False
    
    def close(self):
        """Close serial connection."""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("\nConnection closed")

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Sword of Secrets firmware sanity test')
    parser.add_argument('port', nargs='?', default=SERPORT, help=f'Serial port (default: {SERPORT})')
    parser.add_argument('--reboot', action='store_true', help='Include REBOOT command test (will reset device)')
    args = parser.parse_args()
    
    tester = SanityTest(port=args.port)
    try:
        success = tester.run_all_tests(include_reboot=args.reboot)
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\nUnexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
    finally:
        tester.close()

if __name__ == "__main__":
    main()

