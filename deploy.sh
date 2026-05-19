#!/bin/bash
# =============================================================================
# DrywellDT Multi-Deployment AWS Deploy Script
#
# Usage:
#   ./deploy.sh <deployment_name> [<deployment_name> ...]
#
# Examples:
#   ./deploy.sh Bioretention_truth Bioretention_assimilation
#   ./deploy.sh Bioretention_truth
#
# Behavior:
#   - Builds runner + viewer WASM locally
#   - For each named deployment:
#       * Stops the matching systemd instance (drywelldt@<name>)
#       * Rsyncs deployments/<name>/ to EC2 (excluding runtime dirs)
#       * Always wipes outputs/state/snapshots on EC2 (fresh start)
#       * Generates a per-role viewer at /var/www/drywelldt/<role>/
#       * Installs an nginx server block for outputs (808X) + viewer (908X)
#       * Restarts drywelldt@<name>
#
# Port map (extend here when adding new deployments):
#   Bioretention_truth         -> 8084 outputs, 9084 viewer
#   Bioretention_assimilation  -> 8085 outputs, 9085 viewer
#
# Deployment role is inferred from the name suffix:
#   *_truth         -> forward viewer (charts + SVG)
#   *_assimilation  -> assim viewer (fitness/parameters/comparison tabs)
# =============================================================================

set -e

# --- Configuration ---------------------------------------------------------
EC2_USER="ubuntu"
EC2_HOST="ec2-34-221-236-134.us-west-2.compute.amazonaws.com"
EC2_PUBLIC_IP="34.221.236.134"
PEM_FILE="/home/arash/Dropbox/AWS (Selective Sync Conflict)/ArashLinux.pem"

LOCAL_PROJECT="/home/arash/Projects/DrywellDT"
LOCAL_VIEWER="${LOCAL_PROJECT}/viewer"
QT_LIB_DIR="/home/arash/Qt/6.8.2/gcc_64/lib"
OHQ_LIB_DIR="${LOCAL_PROJECT}/libs/release"

# Project name (from .pro: TARGET = OHTwin) and build output path
# (from .pro: BUILD_DIR = $$PWD/build-qmake, DESTDIR = $$BUILD_DIR/bin).
PRO_FILE="${LOCAL_PROJECT}/OHTwin.pro"
RUNNER_BINARY_NAME="OHTwin"
RUNNER_BUILD="${LOCAL_PROJECT}/build-qmake"
RUNNER_BIN="${RUNNER_BUILD}/bin/${RUNNER_BINARY_NAME}"

# Where each deployment lives on EC2 (mirrors local layout)
EC2_ROOT="/home/ubuntu/drywelldt"
EC2_BIN="${EC2_ROOT}/bin"
EC2_LIB="${EC2_ROOT}/lib"
EC2_DEPLOY_ROOT="${EC2_ROOT}/deployments"
EC2_WWW="/var/www/drywelldt"

VIEWER_BUILD="${LOCAL_VIEWER}/build/WebAssembly_Qt_6_8_2_single_threaded-Release"

# Templates the script can sed/sub on
LOCAL_VIEWER_TEMPLATE_FWD="${LOCAL_VIEWER}/viewer_config_forward_template.json"
LOCAL_VIEWER_TEMPLATE_ASM="${LOCAL_VIEWER}/viewer_config_assimilation_template.json"

# --- Port map -------------------------------------------------------------
# Bash associative arrays for outputs and viewer ports, keyed by deployment.
declare -A OUTPUTS_PORT=(
    [Bioretention_truth]=8084
    [Bioretention_assimilation]=8085
)
declare -A VIEWER_PORT=(
    [Bioretention_truth]=9084
    [Bioretention_assimilation]=9085
)

# --- Convenience ----------------------------------------------------------
SSH_OPTS=(-i "${PEM_FILE}" "${EC2_USER}@${EC2_HOST}")
SCP_OPTS=(-i "${PEM_FILE}")

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'
log()   { echo -e "${GREEN}[deploy]${NC} $1"; }
warn()  { echo -e "${YELLOW}[deploy]${NC} $1"; }
err()   { echo -e "${RED}[deploy]${NC} $1"; exit 1; }
section(){ echo -e "${GREEN}[deploy]${NC} =================================================="; \
           echo -e "${GREEN}[deploy]${NC} $1"; \
           echo -e "${GREEN}[deploy]${NC} =================================================="; }

# Return "truth" or "assimilation" based on a deployment name's suffix.
role_of() {
    local name="$1"
    case "$name" in
        *_truth)          echo truth ;;
        *_assimilation)   echo assimilation ;;
        *) err "Deployment name '$name' must end in _truth or _assimilation" ;;
    esac
}

# Verify ports are defined for a deployment.
ports_for() {
    local name="$1"
    if [[ -z "${OUTPUTS_PORT[$name]}" || -z "${VIEWER_PORT[$name]}" ]]; then
        err "No port assignment for '$name'. Add it to OUTPUTS_PORT and VIEWER_PORT in this script."
    fi
}

# =============================================================================
# Step 0 — Parse arguments
# =============================================================================
if [[ $# -lt 1 ]]; then
    err "Usage: $0 <deployment_name> [<deployment_name> ...]"
fi
DEPLOYMENTS=("$@")

section "Deploying: ${DEPLOYMENTS[*]}"

# Pre-flight: every deployment must exist locally and have port assignments.
for d in "${DEPLOYMENTS[@]}"; do
    [[ -d "${LOCAL_PROJECT}/deployments/${d}" ]] || \
        err "Local deployment not found: ${LOCAL_PROJECT}/deployments/${d}"
    [[ -f "${LOCAL_PROJECT}/deployments/${d}/config.json" ]] || \
        err "No config.json in deployments/${d}"
    ports_for "$d"
    role_of  "$d" >/dev/null   # validates suffix
done

# =============================================================================
# Step 1 — Build runner (Release, desktop)
# =============================================================================
section "Building OHTwin runner (Release)..."
mkdir -p "${RUNNER_BUILD}"
cd "${RUNNER_BUILD}"
/home/arash/Qt/6.8.2/gcc_64/bin/qmake \
    "${PRO_FILE}" \
    CONFIG+=release CONFIG-=debug
make -j"$(nproc)"
[[ -x "${RUNNER_BIN}" ]] || err "Build produced no binary at ${RUNNER_BIN}"
log "Runner built: ${RUNNER_BIN}"

# =============================================================================
# Step 2 — Build viewer (WebAssembly Release)
# =============================================================================
section "Building viewer (WebAssembly Release)..."
mkdir -p "${VIEWER_BUILD}"
cd "${VIEWER_BUILD}"
/home/arash/Qt/6.8.2/wasm_singlethread/bin/qmake \
    "${LOCAL_VIEWER}/OHTwinViewer.pro" \
    CONFIG+=release CONFIG-=debug
make -j"$(nproc)"
log "Viewer built."

# =============================================================================
# Step 3 — Stage shared libs
# =============================================================================
section "Collecting shared libraries..."
BUNDLE_DIR="/tmp/drywelldt_bundle"
rm -rf "${BUNDLE_DIR}"
mkdir -p "${BUNDLE_DIR}/lib"

for lib in libQt6Network.so.6 libQt6Core.so.6 \
           libicui18n.so.73 libicuuc.so.73 libicudata.so.73; do
    if [[ -f "${QT_LIB_DIR}/${lib}" ]]; then
        cp "${QT_LIB_DIR}/${lib}" "${BUNDLE_DIR}/lib/"
    else
        warn "Qt lib not found: ${lib}"
    fi
done
if [[ -d "${OHQ_LIB_DIR}" ]]; then
    cp "${OHQ_LIB_DIR}"/*.so* "${BUNDLE_DIR}/lib/" 2>/dev/null || true
fi
mkdir -p "${BUNDLE_DIR}/plugins/tls"
cp "/home/arash/Qt/6.8.2/gcc_64/plugins/tls/libqopensslbackend.so" \
   "${BUNDLE_DIR}/plugins/tls/"

# =============================================================================
# Step 4 — Stage wrapper, systemd unit, viewer config templates
# =============================================================================
section "Staging wrapper, systemd unit, and viewer configs..."

# Wrapper that systemd will invoke for instance "%i".
# Sets LD_LIBRARY_PATH, points the binary at /home/ubuntu/drywelldt/deployments/<name>/.
cat > "${BUNDLE_DIR}/run_drywelldt.sh" << 'WRAPPER'
#!/bin/bash
# Invoked by systemd: run_drywelldt.sh <deployment_name>
# The deployment name is provided by the @ instance suffix.
set -e
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOYMENT="$1"
if [[ -z "$DEPLOYMENT" ]]; then
    echo "Usage: $(basename "$0") <deployment_name>" >&2
    exit 1
fi
export LD_LIBRARY_PATH="${DIR}/../lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${DIR}/../plugins"
exec "${DIR}/OHTwin" \
    --deployment "/home/ubuntu/drywelldt/deployments/${DEPLOYMENT}"
WRAPPER
chmod +x "${BUNDLE_DIR}/run_drywelldt.sh"

# systemd template unit: drywelldt@.service
# %i is the instance name (the deployment name)
cat > "${BUNDLE_DIR}/drywelldt@.service" << 'UNIT'
[Unit]
Description=DrywellDT Digital Twin (%i)
After=network.target

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/home/ubuntu/drywelldt/bin
ExecStart=/home/ubuntu/drywelldt/bin/run_drywelldt.sh %i
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=drywelldt-%i

[Install]
WantedBy=multi-user.target
UNIT

# Forward-viewer config template (used when role == truth).
# Uses relative URLs (path-based routing under the domain) — works regardless
# of whether the page is accessed via the domain or via IP+port, and avoids
# CORS issues from cross-origin fetches.
cat > "${BUNDLE_DIR}/viewer_config_forward.template.json" << 'EOF'
{
    "mode": "forward",
    "refresh_seconds": 300,
    "forward": {
        "csv_url":                "/__DEPLOYMENT__/outputs/selected_output.csv",
        "viz_url":                "/__DEPLOYMENT__/outputs/viz.svg",
        "viz_state_url":          "/__DEPLOYMENT__/outputs/viz_state.json",
        "forecast_viz_url":       "/__DEPLOYMENT__/outputs/forecast_viz.svg",
        "forecast_viz_state_url": "/__DEPLOYMENT__/outputs/forecast_viz_state.json"
    }
}
EOF

# Assim-viewer config template (used when role == assimilation).
# Uses relative URLs.
cat > "${BUNDLE_DIR}/viewer_config_assimilation.template.json" << 'EOF'
{
    "mode": "assimilation",
    "refresh_seconds": 10,
    "assimilation": {
        "ga_merged_url":    "/__DEPLOYMENT__/outputs/calibration/ga_output_merged.txt",
        "observed_csv_url": "/__TRUTH_DEPLOYMENT__/outputs/selected_output.csv",
        "modeled_csv_url":  "/__DEPLOYMENT__/outputs/selected_output.csv",
        "x_axis": "t_now",
        "fitness_metrics": ["nse", "nmse"],
        "parameter_panel": {
            "show_population_band": true
        }
    }
}
EOF

# =============================================================================
# Step 5 — Set up EC2: shared dirs + stop services we're about to redeploy
# =============================================================================
section "Preparing EC2 environment..."

ssh "${SSH_OPTS[@]}" bash -s -- "${DEPLOYMENTS[@]}" << 'ENDSSH'
set -e
mkdir -p /home/ubuntu/drywelldt/{bin,lib,plugins/tls,deployments}
sudo mkdir -p /var/www/drywelldt
sudo chown -R ubuntu:ubuntu /var/www/drywelldt

# Stop systemd instances we're about to redeploy. The unit file may not
# exist yet on a fresh box; that's fine.
for d in "$@"; do
    sudo systemctl stop "drywelldt@${d}.service" 2>/dev/null || true
done
ENDSSH

# =============================================================================
# Step 6 — Push runner binary, libs, wrapper, systemd unit
# =============================================================================
section "Pushing shared artifacts (runner, libs, wrapper, systemd unit)..."

scp "${SCP_OPTS[@]}" "${RUNNER_BIN}" \
    "${EC2_USER}@${EC2_HOST}:${EC2_BIN}/"
scp "${SCP_OPTS[@]}" "${BUNDLE_DIR}/run_drywelldt.sh" \
    "${EC2_USER}@${EC2_HOST}:${EC2_BIN}/"
scp "${SCP_OPTS[@]}" "${BUNDLE_DIR}"/lib/* \
    "${EC2_USER}@${EC2_HOST}:${EC2_LIB}/"
scp "${SCP_OPTS[@]}" "${BUNDLE_DIR}"/plugins/tls/libqopensslbackend.so \
    "${EC2_USER}@${EC2_HOST}:${EC2_ROOT}/plugins/tls/"

# Resources (OHQ JSON metadata files etc.)
ssh "${SSH_OPTS[@]}" "mkdir -p ${EC2_BIN}/resources"
scp "${SCP_OPTS[@]}" "${LOCAL_PROJECT}/resources"/*.json \
                     "${LOCAL_PROJECT}/resources"/*.list \
    "${EC2_USER}@${EC2_HOST}:${EC2_BIN}/resources/"

# systemd template unit
scp "${SCP_OPTS[@]}" "${BUNDLE_DIR}/drywelldt@.service" \
    "${EC2_USER}@${EC2_HOST}:/tmp/drywelldt@.service"
ssh "${SSH_OPTS[@]}" bash -s << 'ENDSSH'
set -e
sudo mv /tmp/drywelldt@.service /etc/systemd/system/drywelldt@.service
sudo systemctl daemon-reload
chmod +x /home/ubuntu/drywelldt/bin/OHTwin
chmod +x /home/ubuntu/drywelldt/bin/run_drywelldt.sh
ENDSSH

# =============================================================================
# Step 7 — Per-deployment push
# =============================================================================
for d in "${DEPLOYMENTS[@]}"; do
    role=$(role_of "$d")
    outputs_port="${OUTPUTS_PORT[$d]}"
    viewer_port="${VIEWER_PORT[$d]}"
    truth_port="${OUTPUTS_PORT[Bioretention_truth]}"   # assim viewer needs this

    section "Deploying ${d} (role=${role}, outputs=${outputs_port}, viewer=${viewer_port})"

    # 7a — rsync the deployment directory, excluding runtime dirs.
    log "  Rsyncing deployment directory..."
    rsync -av --delete \
        --exclude='outputs/' \
        --exclude='state/' \
        --exclude='snapshots/' \
        -e "ssh -i \"${PEM_FILE}\"" \
        "${LOCAL_PROJECT}/deployments/${d}/" \
        "${EC2_USER}@${EC2_HOST}:${EC2_DEPLOY_ROOT}/${d}/"

    # 7b — wipe runtime dirs on EC2, re-create empty.
    log "  Resetting runtime dirs (outputs, state, snapshots)..."
    ssh "${SSH_OPTS[@]}" bash -s -- "${d}" << 'ENDSSH'
set -e
D="$1"
rm -rf "/home/ubuntu/drywelldt/deployments/${D}/outputs"
rm -rf "/home/ubuntu/drywelldt/deployments/${D}/state"
rm -rf "/home/ubuntu/drywelldt/deployments/${D}/snapshots"
mkdir -p "/home/ubuntu/drywelldt/deployments/${D}/outputs"
mkdir -p "/home/ubuntu/drywelldt/deployments/${D}/state"
mkdir -p "/home/ubuntu/drywelldt/deployments/${D}/snapshots"
chmod -R o+rx "/home/ubuntu/drywelldt/deployments/${D}/outputs"
ENDSSH

    # 7c — generate per-deployment viewer.
    log "  Generating viewer (role=${role})..."
    VIEW_STAGE="${BUNDLE_DIR}/viewers/${d}"
    rm -rf "${VIEW_STAGE}"
    mkdir -p "${VIEW_STAGE}"
    cp "${VIEWER_BUILD}/DrywellDTViewer.html" \
       "${VIEWER_BUILD}/DrywellDTViewer.js"   \
       "${VIEWER_BUILD}/DrywellDTViewer.wasm" \
       "${VIEWER_BUILD}/qtloader.js"          \
       "${VIEWER_BUILD}/qtlogo.svg"           \
       "${VIEW_STAGE}/"

    if [[ "$role" == "truth" ]]; then
        sed -e "s|__DEPLOYMENT__|${d}|g" \
            "${BUNDLE_DIR}/viewer_config_forward.template.json" \
            > "${VIEW_STAGE}/config.json"
    else
        sed -e "s|__DEPLOYMENT__|${d}|g" \
            -e "s|__TRUTH_DEPLOYMENT__|Bioretention_truth|g" \
            "${BUNDLE_DIR}/viewer_config_assimilation.template.json" \
            > "${VIEW_STAGE}/config.json"
    fi

    # 7d — push viewer to /var/www/drywelldt/<role>/
    log "  Pushing viewer files to /var/www/drywelldt/${role}/..."
    ssh "${SSH_OPTS[@]}" "mkdir -p /var/www/drywelldt/${role} && rm -f /var/www/drywelldt/${role}/*"
    scp "${SCP_OPTS[@]}" "${VIEW_STAGE}"/* \
        "${EC2_USER}@${EC2_HOST}:/var/www/drywelldt/${role}/"

    # 7e — install nginx server blocks (one for outputs, one for viewer).
    log "  Installing nginx server blocks (${outputs_port} outputs, ${viewer_port} viewer)..."
    ssh "${SSH_OPTS[@]}" bash -s -- \
        "${d}" "${role}" "${outputs_port}" "${viewer_port}" << 'ENDSSH'
set -e
D="$1"; ROLE="$2"; OUT_PORT="$3"; VIEW_PORT="$4"

sudo tee "/etc/nginx/sites-available/drywelldt-${D}" > /dev/null << EOF
# Outputs server for ${D}
server {
    listen ${OUT_PORT};
    server_name _;
    location /outputs/ {
        alias /home/ubuntu/drywelldt/deployments/${D}/outputs/;
        add_header Access-Control-Allow-Origin * always;
        add_header Cache-Control no-cache always;
        add_header Cross-Origin-Opener-Policy same-origin always;
        add_header Cross-Origin-Embedder-Policy require-corp always;
    }
}

# Viewer page for ${D}
server {
    listen ${VIEW_PORT};
    server_name _;
    root /var/www/drywelldt/${ROLE};
    index DrywellDTViewer.html;
    location / {
        try_files \$uri \$uri/ =404;
        add_header Cross-Origin-Opener-Policy same-origin always;
        add_header Cross-Origin-Embedder-Policy require-corp always;
    }
}
EOF

sudo ln -sf "/etc/nginx/sites-available/drywelldt-${D}" \
            "/etc/nginx/sites-enabled/drywelldt-${D}"
ENDSSH
done

# =============================================================================
# Step 8 — Validate nginx, restart services
# =============================================================================
section "Validating nginx and restarting services..."
ssh "${SSH_OPTS[@]}" bash -s -- "${DEPLOYMENTS[@]}" << 'ENDSSH'
set -e
sudo nginx -t
sudo systemctl reload nginx

# Restart and enable each deployment's systemd instance.
for d in "$@"; do
    sudo systemctl enable "drywelldt@${d}.service"
    sudo systemctl restart "drywelldt@${d}.service"
done

sleep 3

for d in "$@"; do
    echo "----- drywelldt@${d}.service -----"
    sudo systemctl status "drywelldt@${d}.service" --no-pager | head -15
done
ENDSSH

# =============================================================================
# Done — print URLs
# =============================================================================
section "Deployment complete"
echo
for d in "${DEPLOYMENTS[@]}"; do
    role=$(role_of "$d")
    op="${OUTPUTS_PORT[$d]}"
    vp="${VIEWER_PORT[$d]}"
    echo "  ${d} (${role}):"
    echo "    Viewer:  http://${EC2_PUBLIC_IP}:${vp}/DrywellDTViewer.html"
    echo "    Outputs: http://${EC2_PUBLIC_IP}:${op}/outputs/"
    echo
done
echo "To watch logs:"
echo "  ssh -i \"${PEM_FILE}\" ${EC2_USER}@${EC2_HOST}"
for d in "${DEPLOYMENTS[@]}"; do
    echo "  sudo journalctl -u drywelldt@${d} -f"
done
