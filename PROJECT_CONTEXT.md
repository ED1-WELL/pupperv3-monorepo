# Pupper V3 Project Context

This document is a reference for AI assistants. It describes the hardware, software architecture, and key file locations for the Pupper V3 quadruped robot project.

---

## Hardware

### Onboard Compute
- **Main computer:** Raspberry Pi 5
- **OS:** Ubuntu 24 (custom image built with Packer, located in `infra/pupper_image_builder/`)
- **AI accelerator:** Hailo-8L M.2 via Raspberry Pi AI HAT+

### Actuators
- **Motors:** Steadywin GIM4305 brushless motors (12 total — 3 per leg)
- **Driver:** SHS driver board (custom control board, interfaced via SPI)
- **Hardware interface code:** `ros2_ws/src/control_board_hardware_interface/`

### Sensors
- **IMU:** BNO055, connected via SPI
  - Driver: `ros2_ws/src/control_board_hardware_interface/rt/rt_bno055.cpp`
  - Manager: `ros2_ws/src/control_board_hardware_interface/imu_manager.cpp`
- **Camera:** Fisheye lens camera (libcamera-based)
  - Fisheye calibration: `ros2_ws/src/hailo/fisheye_utils.py`
  - Camera config: `ros2_ws/src/hailo/camera_params.yaml`

### Audio
- USB microphone + USB/wired speaker
- Audio config: `infra/pupper_image_builder/asound.conf`

### Gamepad
- PS5 DualSense controller (Bluetooth)
- Pairing script: `robot/utils/pair_ps5_controller.sh`
- L1 = start bag recording, R1 = stop bag recording

### Off-Robot Compute
- **RL training:** Gaming desktop (local) + Google Colab
- **RL training code:** `ai/rl/`

---

## Robot Kinematics

- **12 joints** (4 legs × 3 DOF each)
- Joint naming convention: `leg_{front|back}_{r|l}_{1|2|3}`
  - Joint 1 = hip abduction, Joint 2 = hip flexion, Joint 3 = knee

| Parameter | Value |
|-----------|-------|
| Controller rate | 520 Hz |
| Effective policy rate | 52 Hz (repeat_action=10) |
| KP (stiffness) | 7.5 |
| KD (damping) | 0.25 |
| Default resting position | `[0.26, 0.0, -0.52, -0.26, 0.0, 0.52, 0.26, 0.0, -0.52, -0.26, 0.0, 0.52]` |

---

## Software Stack

### ROS2
- **Version:** ROS2 Jazzy
- **Workspace:** `ros2_ws/`
- **Build:** `ros2_ws/build.sh` (uses colcon, RelWithDebInfo)

### Package Manager
- Python: `uv` (never modify PATH manually)
- Rust: Cargo

---

## ROS2 Packages

### Motion Control

| Package | Path | Description |
|---------|------|-------------|
| `neural_controller` | `ros2_ws/src/neural_controller/` | Runs trained RL locomotion policy on-robot at real-time |
| `animation_controller_py` | `ros2_ws/src/animation_controller_py/` | Plays back pre-recorded joint trajectories from CSV files |
| `real2sim_controller` | `ros2_ws/src/real2sim_controller/` | Mirrors real robot state into a simulation |

**Neural controller key files:**
- `ros2_ws/src/neural_controller/neural_controller.cpp` — main C++ node
- `ros2_ws/src/neural_controller/launch/config.yaml` — gains, rates, joint config
- Trained policy JSON files live in `ros2_ws/src/neural_controller/`:
  - `policy_latest.json` (1.3 MB, current default)
  - Several larger archived policies (~38 MB each)
- Uses RTNeural (submodule) for real-time neural inference

**Animation system key files:**
- `ros2_ws/src/animation_controller_py/animation_controller.py`
- CSV animations: `ros2_ws/src/animation_controller_py/launch/animations/`
  - Available: sit, lie, swim, spider, sneeze, pee, push_up, twerk, downward_dog, upward_dog, superman, and others

### Hardware Interface

| Package | Path | Description |
|---------|------|-------------|
| `control_board_hardware_interface` | `ros2_ws/src/control_board_hardware_interface/` | ros2_control hardware interface for SHS driver board and BNO055 IMU |
| `imu_to_tf` | `ros2_ws/src/imu_to_tf/` | Converts IMU readings to ROS2 TF frames |

### Vision / Perception

| Package | Path | Description |
|---------|------|-------------|
| `hailo` | `ros2_ws/src/hailo/` | Object detection via Hailo-8L NPU |
| `person_follower` | `ros2_ws/src/person_follower/` | Visual servoing to follow a detected person |

**Hailo key files:**
- `ros2_ws/src/hailo/hailo_detection.py` — detection node
- `ros2_ws/src/hailo/hailo_inference.py` — model inference
- `ros2_ws/src/hailo/hailo_depth.py` — depth estimation
- `ros2_ws/src/hailo/mock_camera.py` — camera simulator for dev/testing
- `ros2_ws/src/hailo/fisheye_utils.py` — fisheye lens undistortion

### Joystick / Command Arbitration

| Package | Path | Description |
|---------|------|-------------|
| `joy_utils` | `ros2_ws/src/joy_utils/` | Emergency stop controller |
| `cmd_vel_mux` | `ros2_ws/src/cmd_vel_mux/` | Multiplexes multiple velocity command sources |

### AI / Voice Assistant

| Package | Path | Description |
|---------|------|-------------|
| `openai_bridge` | `ros2_ws/src/openai_bridge/` | OpenAI Realtime API client as a ROS2 node |
| `llm_websocket_server` | `ros2_ws/src/llm_websocket_server/` | WebSocket bridge for LLM communication |

**OpenAI bridge key files:**
- `ros2_ws/src/openai_bridge/ros_node.py` — ROS2 wrapper
- `ros2_ws/src/openai_bridge/realtime_client_class.py` — Realtime API client
- `ros2_ws/src/openai_bridge/audio_util.py` — audio I/O

### Expressions / Face

| Package | Path | Description |
|---------|------|-------------|
| `pupper_feelings` | `ros2_ws/src/pupper_feelings/` | Eye and ear servo control, expression GUI |

- `ros2_ws/src/pupper_feelings/face_control.py` — eye animations
- `ros2_ws/src/pupper_feelings/ear_control.py` — ear movement
- `ros2_ws/src/pupper_feelings/robot_htop.py` — system stats display

### Data Recording

| Package | Path | Description |
|---------|------|-------------|
| `bag_recorder` | `ros2_ws/src/bag_recorder/` | Joystick-triggered MCAP bag recording |

- Output directory: `~/bags/`
- Trigger: hold L1 for 1 second to start, R1 to stop

### Robot Description / Simulation

| Package | Path | Description |
|---------|------|-------------|
| `pupper_v3_description` | `ros2_ws/src/pupper_v3_description/` | URDF models, MuJoCo XML variants, mesh assets |
| `pupperv3_mujoco_sim` | `ros2_ws/src/pupperv3_mujoco_sim/` | MuJoCo physics simulation hardware interface |

---

## Desktop GUI (Rust)

**Location:** `pupper-rs/`  
**Framework:** egui / eframe  
**Purpose:** Status dashboard running on the robot's display

Key files:
- `pupper-rs/src/main.rs` — application entry point
- `pupper-rs/src/eyes/` — eye rendering, blinking, tracking animation
- `pupper-rs/src/system/` — battery, CPU, network, service monitoring
- `pupper-rs/config.toml` — GUI configuration (eye tracking, blink intervals, battery thresholds)

---

## AI / LLM Subsystems (`ai/`)

### RL Training (`ai/rl/`)
- Training environment: MuJoCo MJX (GPU-accelerated)
- Runs on: gaming desktop or Google Colab
- Main training script: `ai/rl/pupper_mjx_rl_training.py`
- Config: `ai/rl/conf/config.yaml` (Hydra-based)
- Experiment tracking: Weights & Biases (wandb)
- Trained policies exported as JSON, deployed to `ros2_ws/src/neural_controller/`

### Voice UI (`ai/llm-ui/`)

| Subproject | Path | Description |
|------------|------|-------------|
| `agent-starter-python` | `ai/llm-ui/agent-starter-python/` | LiveKit Agents-based voice assistant (Python) |
| `live-audio` | `ai/llm-ui/live-audio/` | TypeScript/React frontend + Python backend for live audio |
| `moonshine-test` | `ai/llm-ui/moonshine-test/` | Moonshine speech-to-text experiments |
| `ui-rs` | `ai/llm-ui/ui-rs/` | Rust UI variant |

`live-audio` key files:
- `ai/llm-ui/live-audio/robot_server.py` — Python backend
- `ai/llm-ui/live-audio/index.tsx` — React frontend
- `ai/llm-ui/live-audio/audio-manager.ts` — audio stream handling

---

## Infrastructure (`infra/`)

**Image builder:** `infra/pupper_image_builder/`  
Builds Raspberry Pi 5 OS images using HashiCorp Packer.

Three image variants:
| Image | Packer file | Purpose |
|-------|-------------|---------|
| Base | `pios_base_arm64.pkr.hcl` | OS + ROS2 |
| Full | `pios_full_arm64.pkr.hcl` | Base + robot control stack |
| AI | `pios_ai_arm64.pkr.hcl` | Full + LLM/audio/vision |

Provisioning scripts: `provision_pios_base.sh`, `provision_pios_full.sh`, `provision_pios_ai.sh`

---

## Robot Startup & Services (`robot/`)

- **Main launch:** `robot/utils/robot.sh` (or `tmux_run_robot.sh` for tmux session)
- **Systemd service:** `robot/utils/robot.service`
- **Auto-start install:** `robot/utils/install_robot_auto_start_service.sh`
- **Battery monitor:** `robot/utils/check_batt_voltage.py`, `robot/utils/battery_monitor.service`
- **UI launch:** `robot/start_ui.sh`
- **Stop all:** `stop_all_services.sh` (repo root)

---

## Scripts & Utilities (`scripts/`)

| Script | Path | Description |
|--------|------|-------------|
| Animation editor | `scripts/animation_editor/main.py` | GUI tool for creating/editing animation CSVs |
| MCAP tools | `scripts/mcap_tools/main.py` | Bag file processing utilities |
| MCAP to CSV | `scripts/mcap_to_csv.py` | Convert bag recordings to CSV (`-s START -e END` flags) |
| Record bags | `scripts/record_all_except_raw_camera.sh` | ROS2 bag recording (excludes raw camera) |

---

## Recorded Motion Data (`bags/`)

MCAP format bag files recorded via joystick. Used as source material for the animation system.  
Animations extracted and saved to `ros2_ws/src/animation_controller_py/launch/animations/`.

---

## Coding Guidelines

- **No try/except fallbacks** — warn about failure modes instead
- **Package management:** use `uv` for Python; never manually modify PATH in files
- **Build:** `colcon build` from `ros2_ws/`, or run `ros2_ws/build.sh`
- **Logs:** `llm_logs.sh` at repo root for LLM interaction logging
