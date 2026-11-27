import unittest
from pathlib import Path

from merge_mod import normalized_content


class NormalizedContentTests(unittest.TestCase):
    def test_mod_json_ignores_formatting_and_newline_differences(self):
        windows = b'{\r\n  "version": "1.5.1",\r\n  "id": "example"\r\n}\r\n'
        unix = b'{\n"id":"example","version":"1.5.1"\n}\n'

        self.assertEqual(
            normalized_content("mod.json", windows, Path("windows.dusk")),
            normalized_content("mod.json", unix, Path("linux.dusk")),
        )

    def test_mod_json_still_detects_value_changes(self):
        first = b'{"version":"1.5.1"}'
        second = b'{"version":"1.5.2"}'

        self.assertNotEqual(
            normalized_content("mod.json", first, Path("first.dusk")),
            normalized_content("mod.json", second, Path("second.dusk")),
        )

    def test_other_entries_remain_byte_for_byte(self):
        content = b"first line\r\nsecond line\r\n"

        self.assertEqual(
            normalized_content("res/config.txt", content, Path("bundle.dusk")),
            content,
        )


if __name__ == "__main__":
    unittest.main()
