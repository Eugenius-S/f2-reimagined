"""Check changed evidence under runs/ against per-file and image-total limits."""
import subprocess
import sys

LIMIT = 1024 * 1024
IMAGE_LIMIT = 2 * LIMIT
IMAGE_EXTENSIONS = ('.png', '.jpg', '.jpeg', '.gif', '.webp', '.bmp', '.tif', '.tiff', '.avif')


def git(*args):
    return subprocess.run(['git', *args], check=True, capture_output=True).stdout


def main():
    base = sys.argv[1]
    changes = iter(git('diff', '--name-status', '-z', '-M', '--diff-filter=AMR',
                       f'{base}...HEAD').split(b'\0'))
    over = []
    image_total = 0
    for status in changes:
        if not status:
            break
        name = next(changes).decode('utf-8', 'surrogateescape')
        if status.startswith(b'R'):
            name = next(changes).decode('utf-8', 'surrogateescape')
            if status == b'R100':
                continue
        if not name.startswith('runs/'):
            continue
        size = int(git('cat-file', '-s', f'HEAD:{name}'))
        if size > LIMIT:
            over.append((size, name))
        if name.lower().endswith(IMAGE_EXTENSIONS):
            image_total += size
    for size, name in over:
        print(f'{name}: {size} bytes, over the 1 MiB per-file limit')
    if image_total > IMAGE_LIMIT:
        print(f'Changed images under runs/: {image_total} bytes, over the 2 MiB total limit')
    if over or image_total > IMAGE_LIMIT:
        print('Shrink the evidence or store larger proof in the evidence release; see AGENTS.md.')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
