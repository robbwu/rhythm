# Rhythm Web App Deployment

This document outlines how the GitHub Actions workflow deploys the browser-based
Rhythm transpiler and how to configure the target host to serve the static
assets.

## Continuous deployment workflow

The `.github/workflows/deploy-web.yml` workflow builds the browser bundle inside
the official Emscripten container and synchronises the generated files to the
Rhythm host on every push to `main`.

1. Check out the repository.
2. Install `rsync` and the OpenSSH client inside the container image.
3. Run `emcmake cmake -S . -B build/web` to configure the WebAssembly build.
4. Build the `transpose_wasm` target, which produces `transpose_wasm.js`,
   `transpose_wasm.wasm`, and copies the static assets from `web/` into the
   build directory.
5. Stage the contents of `build/web/` for deployment.
6. Authenticate with the deployment server using the
   `COSC3320_RHYTHMBOT_SSH` private key and add the host fingerprint to the
   `known_hosts` file.
7. Use `rsync --delete` to mirror the staged files into
   `/home/rhythmbot/rhythm-web/` on the remote machine. The directory is wiped
   of stale files before copying the fresh build so the server always exposes
   the latest assets.

To activate the workflow you need the following repository secrets:

- `COSC3320_RHYTHMBOT_SSH` – private key with write access to the host.
- `COSC_RHYTHMBOT_SERVER_HOST` – hostname or IP address of the deployment
  server.
- `COSC_RHYTHMBOT_SERVER_USER` – SSH user allowed to write to
  `/home/rhythmbot/rhythm-web/`.

## Preparing the host

The deployment expects an SSH-accessible account (e.g. `rhythmbot`) with write
access to `/home/rhythmbot/rhythm-web/`. The following steps show how to prepare
an Ubuntu or Debian host using Nginx to serve the site. Adjust paths as needed
for your environment.

1. Install system dependencies and create the deploy directory:

   ```bash
   sudo apt update
   sudo apt install -y nginx
   sudo mkdir -p /home/rhythmbot/rhythm-web
   sudo chown -R rhythmbot:rhythmbot /home/rhythmbot/rhythm-web
   ```

2. Configure Nginx to serve the static files:

   ```bash
   sudo tee /etc/nginx/sites-available/rhythm-web <<'CONF'
   server {
       listen 80;
       server_name rhythm.example.com; # replace with your hostname

       root /home/rhythmbot/rhythm-web;
       index index.html;

       location / {
           try_files $uri $uri/ =404;
       }
   }
   CONF

   sudo ln -sf /etc/nginx/sites-available/rhythm-web \
       /etc/nginx/sites-enabled/rhythm-web
   sudo nginx -t
   sudo systemctl reload nginx
   ```

3. Add the deploy key's public half to the SSH account's
   `~/.ssh/authorized_keys` so the workflow can log in. Ensure the directory has
   `700` permissions and the file `600` permissions.

Once these steps are complete, every successful push to `main` will rebuild the
web assets and publish them to the server automatically. If you need to trigger
an ad-hoc deployment without pushing a commit, run the workflow manually from
GitHub's **Actions** tab via the `workflow_dispatch` event.

## Local smoke test

After building the WebAssembly bundle locally, you can verify that the
generated `transpose_wasm` artefacts load and compile Rhythm programs without
starting a browser. Run the Node.js helper script from the repository root and
point it at the build output directory (defaults to `build/web`):

```bash
node scripts/check-transpose-wasm.mjs [path/to/build/web]
```

The script imports `transpose_wasm.js` inside a lightweight, browser-like
environment, supplies a local `locateFile` implementation that resolves the
accompanying `.wasm`, and executes a smoke test that compiles a small Rhythm
snippet. It exits with a non-zero status and prints the captured stderr if
instantiation or compilation fails, making it easy to reproduce loader issues
in CI or on a developer workstation. When the wrapper and binary are out of
sync, the helper surfaces a rebuild hint instead of the opaque WebAssembly
`LinkError` seen in the browser console.
