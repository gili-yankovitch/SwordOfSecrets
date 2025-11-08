#!/usr/bin/env python3
"""
Generate a 2D Data Matrix ECC-200 barcode using ISO/IEC 15434 "Format 06" structure.

The barcode contains the following data:
[)>RS06GS1PSWORD-OF-SECRETSGSQ1GS10D2535GS1TL2501GS4LILGS4SS12345RSEOT

Where:
- RS = Record Separator (ASCII 30 / 0x1E)
- GS = Group Separator (ASCII 29 / 0x1D)
- EOT = End of Transmission (ASCII 4 / 0x04)
"""

import argparse
import sys

try:
    from pystrich.datamatrix import DataMatrixEncoder
except ImportError:
    try:
        from datamatrix import DataMatrix
    except ImportError:
        print("Error: Required library not found.")
        print("Please install one of the following:")
        print("  pip install pystrich")
        print("  or")
        print("  pip install python-datamatrix")
        sys.exit(1)


def construct_data_string():
    """
    Construct the ISO/IEC 15434 Format 06 data string with proper control characters.
    """
    # Control characters
    RS = chr(0x1E)  # Record Separator
    GS = chr(0x1D)  # Group Separator
    EOT = chr(0x04)  # End of Transmission
    
    # Build the data string according to Format 06 structure
    data = "[)>" + RS + "06" + GS + "1P" + "SWORD-OF-SECRETS" + GS + "Q1" + GS + "10D2535" + GS + "1T" + "L2501" + GS + "4L" + "IL" + GS + "4S" + "S12345" + RS + EOT
    
    return data


def generate_datamatrix_pystrich(data, output_file=None, scale=10):
    """
    Generate Data Matrix barcode using pystrich library.
    """
    encoder = DataMatrixEncoder(data)
    encoder.save(output_file or "datamatrix.png", cellsize=scale)
    print(f"Data Matrix barcode saved to: {output_file or 'datamatrix.png'}")
    print(f"Data encoded: {repr(data)}")
    print(f"Data length: {len(data)} bytes")


def generate_datamatrix_datamatrix(data, output_file=None):
    """
    Generate Data Matrix barcode using python-datamatrix library.
    """
    from datamatrix import DataMatrix
    import io
    from PIL import Image
    
    dm = DataMatrix()
    dm.encode(data)
    
    # Get the image
    img = dm.get_pil_image()
    
    # Save to file
    output_file = output_file or "datamatrix.png"
    img.save(output_file)
    print(f"Data Matrix barcode saved to: {output_file}")
    print(f"Data encoded: {repr(data)}")
    print(f"Data length: {len(data)} bytes")


def main():
    parser = argparse.ArgumentParser(
        description="Generate a 2D Data Matrix ECC-200 barcode with ISO/IEC 15434 Format 06 data"
    )
    parser.add_argument(
        "-o", "--output",
        default="datamatrix.png",
        help="Output file path (default: datamatrix.png)"
    )
    parser.add_argument(
        "-s", "--scale",
        type=int,
        default=10,
        help="Cell size/scale for pystrich (default: 10)"
    )
    parser.add_argument(
        "--show-data",
        action="store_true",
        help="Display the encoded data string"
    )
    
    args = parser.parse_args()
    
    # Construct the data string
    data = construct_data_string()
    
    if args.show_data:
        print("Data string (with control characters):")
        print(repr(data))
        print("\nData string (hex dump):")
        print(" ".join(f"{ord(c):02X}" for c in data))
        print()
    
    # Try to generate using available library
    try:
        from pystrich.datamatrix import DataMatrixEncoder
        generate_datamatrix_pystrich(data, args.output, args.scale)
    except ImportError:
        try:
            generate_datamatrix_datamatrix(data, args.output)
        except ImportError:
            print("Error: No Data Matrix library found.")
            print("Please install one of:")
            print("  pip install pystrich")
            print("  or")
            print("  pip install python-datamatrix")
            sys.exit(1)


if __name__ == "__main__":
    main()

