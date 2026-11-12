import subprocess\nimport sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
CHECKER = Path(__file__).resolve().with_name('check_runs_size.py')
LIMIT = 1024 * 1024


class CheckRunsSizeTests(unittest.TestCase):
    def test_checks_added_file_sizes_only_under_runs(self):
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)

            def git(*args):
                return subprocess.run(
                    ['git', *args], cwd=repo, check=True,
                    capture_output=True, text=True,
                )

            git('init', '-q', '-b', 'main')
            git('config', 'user.name', 'Test')
            git('config', 'user.email', 'test.invalid')
            (repo / 'runs').mkdir()
            (repo / 'runs' / 'baseline.txt').write_bytes(b'baseline')
            (repo / 'outside.txt').write_bytes(b'baseline')
            git('add', 'runs/baseline.txt', 'outside.txt')
            git('commit', '-qm', 'baseline')
            git('branch', 'base')

            (repo / 'runs' / 'within-limit.txt').write_bytes(b'x' * LIMIT)
            (repo / 'runs' / 'over-limit.txt').write_bytes(b'x' * (LIMIT + 1))
            (repo / 'outside.txt').write_bytes(b'x' * (LIMIT + 10))
            git('add', 'runs/within-limit.txt', 'runs/over-limit.txt', 'outside.txt')
            git('commit', '-qm', 'add files to check')

            result = subprocess.run(
                [sys.executable, str(CHECKER), 'base'], cwd=repo,
                capture_output=True, text=True,
            )

            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn('runs/over-limit.txt', result.stdout)
            self.assertNotIn('runs/within-limit.txt', result.stdout)
            self.assertNotIn('outside.txt', result.stdout)


if __name__ == '__main__':
    unittest.main()




