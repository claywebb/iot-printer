"""Helper functions for flask app."""

def convert_range_to_bitmap(png_data, pos, length):
    """Convert the png image data to 1-bit bitmap format."""
    offset = pos * 8
    if offset >= len(png_data):
        return []

    bw_data = []

    buffer_byte = 0
    byte_index = 0
    for i in range(0, length * 8):
        bit = 0
        if offset+i < len(png_data):
            r, g, b, _ = png_data[offset+i]
            if float((r+b+g))/3 < 255/2:
                bit = 1

        buffer_byte |= bit << (7 - byte_index)
        byte_index += 1

        if byte_index == 8:
            bw_data.append(buffer_byte)
            buffer_byte = 0
            byte_index = 0

    return bw_data
