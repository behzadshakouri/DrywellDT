#!/bin/bash
# =============================================================================
# DrywellDT — Reset Server State
# Stops the runner, clears outputs + state + snapshots on the EC2 host,
# then restarts the service so the runner starts fresh from wall-clock now.
# =============================================================================

set -e

# --- Configuration (matches deploy.sh) ---
EC2_USER="ubuntu"
EC2_HOST="ec2-34-221-236-134.us-west-2.compute.amazonaws.com"
PEM_FILE="/home/arash/Dropbox/AWS (Selective Sync Conflict)/ArashLinux.pem"
SSH_CMD="ssh -i \"${PEM_FILE}\" ${EC2_USER}@${EC2_HOST}"

# --- Colors ---
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'
log()  { echo -e "${GREEN}[reset]${NC} $1"; }
warn() { echo -e "${YELLOW}[reset]${NC} $1"; }
err()  { echo -e "${RED}[reset]${NC} $1"; exit 1; }

log "Resetting DrywellDT state on ${EC2_HOST}..."

eval "${SSH_CMD}" << 'ENDSSH'
set -e

echo "[remote] Stopping drywelldt service..."
sudo systemctl stop drywelldt 2>/dev/null || true

echo "[remote] Clearing outputs..."
rm -rf /home/ubuntu/drywelldt/outputs/*

echo "[remote] Clearing state..."
rm -rf /home/ubuntu/drywelldt/state/*

echo "[remote] Clearing model snapshots..."
rm -rf /home/ubuntu/drywelldt/snapshots/*

echo "[remote] Restarting service..."
sudo systemctl restart drywelldt

# Give it a moment to start solving its first cycle
sleep 4

echo "[remote] Service status:"
sudo systemctl status drywelldt --no-pager --lines=15
ENDSSH

log "=================================================="
log "Reset complete!"
log ""
log "Watch live logs:"
log "  ssh -i \"${PEM_FILE}\" ${EC2_USER}@${EC2_HOST}"
log "  sudo journalctl -u drywelldt -f"
log "=================================================="
