  GNU nano 8.2                                       qr6.py
import qrcode
import argparse

def generate_qr_ascii(data, version=None, stretch=False):
    qr = qrcode.QRCode(
        version=version,  # Use specified version or automatically adjust
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=1,
        border=0,
    )
    qr.add_data(data)
    qr.make(fit=True)
    qr_image = qr.make_image(fill_color="black", back_color="white")

    # Convert the image to ASCII
    ascii_chars = [" ", "█"]  # White space and filled block for ASCII
    ascii_art = ""

    # Access QR code pixels directly for each row and column
    for y in range(qr_image.size[1]):
        row = ""
        for x in range(qr_image.size[0]):
            pixel = qr_image.getpixel((x, y))
            char = ascii_chars[0] if pixel == 255 else ascii_chars[1]
            row += char * 2 if stretch else char  # Duplicate each column if stretch is True
        ascii_art += row + "\n"

    return ascii_art

if __name__ == "__main__":
    # Set up command line argument parsing
    parser = argparse.ArgumentParser(description="Generate ASCII QR code.")
    parser.add_argument("data", help="Data to encode in the QR code")
    parser.add_argument(
        "--stretch", action="store_true",
        help="Stretch horizontally for terminals with rectangular fonts"
    )
    parser.add_argument(
        "--version", type=int, default=None,
        help="Specify QR code version (1 to 40). If omitted, version is auto-detected."
    )
    args = parser.parse_args()

    # Generate and print ASCII QR code
    ascii_qr = generate_qr_ascii(args.data, version=args.version, stretch=args.stretch)
    print(ascii_qr)
