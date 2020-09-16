from pathlib import Path


def _get_font_filenames():
    # Returns the list of font filenames relative to the fonts/ folder.
    # This is then used when adding the fonts in the font database."""
    folder = Path(__file__).parent
    return [str(filename) for filename in folder.rglob("*.ttf")]


FONT_FILENAMES = _get_font_filenames()
