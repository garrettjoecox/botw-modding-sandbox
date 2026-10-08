# devkitPPC + the Python packages WiiXLaunch's scripts need (Pillow for pack_resources.py).
FROM devkitpro/devkitppc:latest
RUN apt-get update && apt-get install -y --no-install-recommends python3-pil && rm -rf /var/lib/apt/lists/* \
 && git config --system --add safe.directory '*'
