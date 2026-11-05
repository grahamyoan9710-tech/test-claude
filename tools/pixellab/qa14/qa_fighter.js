export const meta = {
  name: 'qa-fighter-clips',
  description: 'Review one fighter\'s PixelLab clips page by page, then merge the defects into a fix list',
  phases: [
    { title: 'Review', detail: 'one strict art director per sheet page' },
    { title: 'Merge', detail: 'dedupe, verify the worst against the frames, choose the cheapest fix' },
  ],
}

const S = '/tmp/claude-0/-home-user-test-claude/454a74a0-0344-5f0d-8ed7-e18843be178a/scratchpad'
const f = args.fighter
const dir = `${S}/qa14/${f}`

const DEFECTS = {
  type: 'object',
  properties: {
    defects: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          clip: { type: 'string' },
          frames: { type: 'string', description: 'which frames (e.g. "4-9" or "all")' },
          severity: { type: 'string', enum: ['high', 'medium', 'low'] },
          problem: { type: 'string' },
          fix: { type: 'string', enum: ['regenerate', 'replace_frames', 'erase_effect', 'retime', 'ok_as_is'] },
          detail: { type: 'string', description: 'regenerate: the new prompt; replace_frames: which frame replaces which; erase_effect: the colour of what to erase (blue/yellow/red/warm/all); retime: the impact frame to use' },
        },
        required: ['clip', 'frames', 'severity', 'problem', 'fix', 'detail'],
      },
    },
  },
  required: ['defects'],
}

const common = `You are a strict art director reviewing the frame-by-frame PixelLab animations of ${f.toUpperCase()} for a Smash-like fan game.
The prompt each clip was generated from is in ${dir}/prompts.json (read it): a clip must show that action, by that character, on model
(same outfit, colours, proportions and weapon as in its idle clip), facing right, whole (nothing cut by the frame edge), without stray
objects (cliffs, logs, an invented opponent), and attacks must show the blow clearly (the impact frame given after "impact" is the frame
the game holds while the hitbox is out: it must be the frame where the blow lands). Effects drawn by PixelLab are fine when they stay
small and inside the frame; huge effects (giant beams, tornadoes, explosions) are a defect: the game draws those itself.
For each defect pick the cheapest fix: replace_frames (a few bad frames: say which good frame replaces each), erase_effect (an effect
of one clear colour on top of the character), retime (wrong impact frame: give the right one), regenerate (the action itself is wrong:
write the new prompt, describing only the character, ending with "Only <name> is drawn" when effects must stay out). Report only real
problems a player would notice; ok_as_is for borderline ones is not needed.`

const pages = args.pages  // e.g. ["clips_asta_0.png", ...]
const ingame = args.ingame || []  // e.g. ["page_0.png", ...]

const reviews = await parallel([
  ...pages.map(p => () => agent(`${common}\n\nReview ${dir}/${p} (use the Read tool on it): each row is one clip, its frames left to right with their numbers, the red tick under each frame is the anchor (the feet).`,
    { label: `review:${p}`, phase: 'Review', schema: DEFECTS })),
  ...(ingame.length ? [() => agent(`${common}\n\nReview the in-game pages ${ingame.map(p => `${dir}/${p}`).join(', ')} (Read each): each row is one move as the game plays it, frame numbers of the move under each column, red shapes are the hitboxes, a red bar marks the frames they are out. Report clips that look wrong IN THE GAME: a character of the wrong size, floating or sunk in the floor, jumping sideways between frames, a blow shown far from where the hitboxes are.`,
    { label: 'review:ingame', phase: 'Review', schema: DEFECTS })] : []),
])

const all = reviews.filter(Boolean).flatMap(r => r.defects || [])
log(`${all.length} defects reported for ${f}`)
if (!all.length) return { fighter: f, defects: [] }

const merged = await agent(`${common}\n\nSeveral reviewers looked at different pages of ${f}'s clips (sheets in ${dir}: clips_${f}_*.png, in-game pages page_*.png). Their reports:\n${JSON.stringify(all, null, 1)}\n\nMerge them into one list: one entry per clip (combine the problems of a clip; when two fixes compete keep the one that fixes everything). Open the sheet page of every high-severity clip yourself and drop any report you cannot confirm on the frames. Keep the cheapest fix that really works.`,
  { label: 'merge', phase: 'Merge', schema: DEFECTS })

return { fighter: f, defects: merged ? merged.defects : all }
