namespace ClashHorizon.Core
{
    // src/core/input.hpp: device-independent controller state for one frame.
    // The keyboard, gamepads and the CPU all produce one of these.
    public struct InputState
    {
        public Vec2 stick;    // movement stick, each axis in [-1, 1], +y = down
        public Vec2 cstick;   // "smash stick": instant smash attacks / aerials in a direction
        public bool attack;
        public bool special;
        public bool jump;
        public bool shield;
        public bool grab;
        public bool smash;    // keyboard "smash" key: smash attack / aerial towards the stick (or forward)
        public bool ultimate; // unleashes the ultimate (final smash) once the ultimate meter is full
        public bool weapon;   // summons weapon A, then switches A <-> B
        public bool tapJump;  // pushing the stick up jumps (as in Smash Bros.)
        public bool digital;  // the stick is keys (keyboard): no gentle tilt, so up + attack a little after is an up tilt
    }
}
