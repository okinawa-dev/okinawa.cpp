# 3D Engine Coordinate System Standards

## Right-Handed Coordinate System
Most 3D engines use a right-handed coordinate system, where:

### Base Axes
- **X-axis**: Points to the right
- **Y-axis**: Points up
- **Z-axis**: Points towards you (out of the screen in standard view)

### Default Camera Position and Direction
- **Position**: At origin (0, 0, 0)
- **Looking**: Along negative Z-axis (0, 0, -1)
- **Up vector**: Along positive Y-axis (0, 1, 0)
- **Right vector**: Along positive X-axis (1, 0, 0)

### Rotation Angles (Euler Angles)
- **Yaw** (Y-axis rotation): 
  - Rotates around Y-axis
  - 0° points along negative Z
  - Positive yaw rotates clockwise when viewed from above
  
- **Pitch** (X-axis rotation):
  - Rotates around X-axis
  - 0° is horizontal
  - Positive pitch rotates upward
  - Limited to ±89° to avoid gimbal lock
  
- **Roll** (Z-axis rotation):
  - Rotates around Z-axis (view direction)
  - 0° keeps "up" aligned with Y-axis
  - Positive roll rotates clockwise when looking along view direction

### Vector Relations
- Forward Vector = (sin(yaw)cos(pitch), sin(pitch), cos(yaw)cos(pitch))
- Right Vector = (cos(yaw), 0, -sin(yaw))
- Up Vector = Cross product of Right and Forward

### Important Rules
1. Right × Forward = Down (right-hand rule)
2. Forward × Right = Up
3. Up × Forward = Right
4. Right × Up = Forward

### Common Operations
- Looking left: Positive yaw
- Looking right: Negative yaw
- Looking up: Positive pitch
- Looking down: Negative pitch