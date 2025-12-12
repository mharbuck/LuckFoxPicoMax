import max7219.led as led
import time
from luma.core.virtual import sevensegment

try:
    # Initialize the LED matrix device
    # Set 'cascaded=N' if you have multiple matrices chained together
    device = led.matrix(cascaded=1)

    # You can also use the sevensegment virtual device for digit displays
    # device = sevensegment(device)

    print("Device initialized. Displaying message...")

    # Display a scrolling message
    device.show_message("Hello world!")

    # Example of turning on a specific LED (for matrix displays)
    # device.pixel(x, y, 1) # Sets pixel at (x, y) to ON
    # device.flush() # Renders the buffer to the display

    # Clear the display and exit
    # device.clear()
    # time.sleep(1)

    print("Message displayed. Exiting.")

except Exception as e:
    print(f"An error occurred: {e}")
    print("Ensure SPI is enabled and the library is installed correctly.")
