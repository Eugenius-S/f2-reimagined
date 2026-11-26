import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


CHECKER = Path(__file__).resolve().with_name('check_runs_size.py')
LIMIT = 1024 * 1024


class CheckRunsSizeTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        self.git('init', '-q', '-b', 'main')
        self.git('config', 'user.name', 'Test')
        self.git('config', 'user.email', 'test.invalid')
        (self.repo / 'runs').mkdir()
        self.write('runs/old.png', LIMIT + 1)
        self.write('runs/old-move.png', LIMIT + 2)
        self.write('outside.txt', 1)
        self.write('outside-move.png', LIMIT + 3)
        self.commit('baseline')
        self.git('branch', 'base')

    def git(self, *args):
        return subprocess.run(['git', *args], cwd=self.repo, check=True,
                              capture_output=True)

    def write(self, name, size):
        (self.repo / name).write_bytes(b'x' * size)

    def commit(self, message):
        self.git('add', '-A')
        self.git('commit', '-qm', message)

    def check(self):
        return subprocess.run([sys.executable, str(CHECKER), 'base'], cwd=self.repo,
                              capture_output=True, text=True)

    def test_per_file_limit_and_old_or_outside_files(self):
        self.write('runs/within-limit.txt', LIMIT)
        self.write('outside.txt', LIMIT + 10)
        self.git('mv', 'runs/old-move.png', 'runs/moved.png')
        self.git('mv', 'outside-move.png', 'runs/moved-from-outside.png')
        self.commit('add and move')
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.write('runs/over-limit.txt', LIMIT + 1)
        self.commit('add oversized file')
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('runs/over-limit.txt', result.stdout)
        self.assertNotIn('runs/within-limit.txt', result.stdout)
        self.assertNotIn('runs/moved.png', result.stdout)
        self.assertNotIn('outside.txt', result.stdout)

    def test_image_total_boundary_and_overflow(self):
        self.write('runs/one.PNG', LIMIT)
        self.write('runs/two.jpeg', LIMIT)
        self.commit('exact boundary')
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.write('runs/third.webp', 1)
        self.commit('over total')
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('over the 2 MiB total limit', result.stdout)
        self.assertNotIn('per-file limit', result.stdout)

    def test_type_changes_check_blobs_and_skip_gitlinks(self):
        blob = self.git('hash-object', '-w', '--stdin').stdout.strip().decode()
        self.git('update-index', '--add', '--cacheinfo', f'120000,{blob},runs/type.png')
        self.git('commit', '-qm', 'add symlink to baseline')
        self.git('branch', '-f', 'base')
        self.write('runs/type.png', LIMIT + 1)
        self.git('add', 'runs/type.png')
        self.git('commit', '-qm', 'replace symlink with large file')
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('runs/type.png', result.stdout)
        self.write('runs/type.png', LIMIT)
        self.write('runs/other.png', LIMIT)
        self.write('runs/extra.png', 1)
        self.commit('type change over image total')
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('over the 2 MiB total limit', result.stdout)
        missing_commit = '1' * 40
        self.git('update-index', '--cacheinfo', f'160000,{missing_commit},runs/type.png')
        self.git('commit', '-qm', 'replace with gitlink')
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_modified_images_count_and_nul_delimited_paths(self):
        self.write('runs/old.png', LIMIT)
        self.write('runs/name with spaces and ü.png', LIMIT)
        self.write('runs/extra.GIF', 1)
        self.commit('modify and add images')
        result = self.check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('over the 2 MiB total limit', result.stdout)


if __name__ == '__main__':
    unittest.main()
