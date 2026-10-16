"""Generates the game's sounds with ElevenLabs into a raw folder (skips files already there)."""
import json, os, subprocess, sys
out = sys.argv[1]
KEY = open(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'el', '.key')).read().strip()
VOICE = 'SOYHLrjzK2X1ezoPC6cr'
S = 'anime fighting game sound effect, no music, no voice: '
SFX = {
 'hit_light': ('quick light punch hit, snappy and crisp', 0.5),
 'hit_medium': ('solid medium punch impact with a meaty thud', 0.6),
 'hit_heavy': ('heavy powerful hit impact with a deep bass thump and crunch', 0.9),
 'hit_fire': ('fiery hit, burst of flames whoosh with an impact', 0.9),
 'hit_electric': ('electric shock hit, crackling lightning zap impact', 0.8),
 'hit_slash': ('sharp sword slash hitting, blade slice with a metallic edge', 0.6),
 'swing': ('fast whoosh of a punch swinging through the air', 0.5),
 'jump': ('light jump, quick cloth whoosh and soft hop', 0.5),
 'air_jump': ('magical double jump in mid air, airy whoosh with a soft sparkle', 0.5),
 'land': ('landing on stone ground, soft thud of feet', 0.5),
 'shield_hit': ('hit blocked by an energy shield, glassy resonant impact', 0.6),
 'shield_break': ('energy shield shattering like glass, then a dizzy ringing', 1.5),
 'ko': ('fighter blasted off the screen, huge explosive star burst with a long echo', 2.0),
 'crowd': ('arena crowd cheering excitedly, short burst of applause', 2.0),
 'projectile': ('magical energy projectile fired, quick whoosh', 0.6),
 'grab': ('quickly grabbing an opponent by the clothes, cloth snatch', 0.5),
 'counter': ('counter attack parry, sharp metallic ching with a flash', 0.7),
 'ledge': ('hand grabbing a stone ledge, quick grip', 0.5),
 'menu_move': ('soft short wooden UI tick for moving a cursor in a game menu', 0.5),
 'menu_select': ('bright satisfying confirm chime in a game menu', 0.6),
 'menu_back': ('soft cancel UI sound, short descending blip', 0.5),
 'beep': ('countdown beep, short bright electronic tone', 0.5),
 'go': ('start of a fight, powerful gong hit with a whoosh', 1.2),
 'game_set': ('end of match, huge dramatic orchestral hit with a gong', 2.0),
 'ult_ready': ('power fully charged, rising magical shimmer ending with a bright ding', 1.2),
 'ult_unleash': ('massive power unleashed, energy explosion with a rising roar of wind', 2.0),
 'wind_gust': ('strong gust of wind blowing past, whoosh', 1.0),
 'ice_crack': ('ice crystals forming quickly and cracking', 1.0),
 'petal_swish': ('swirl of flower petals and thin blades swishing through the air', 1.0),
 'hit_holy': ('holy light impact, bright shimmering burst', 1.0),
 'hit_gloom': ('dark energy impact, deep ominous whoosh', 1.0),
 'beam_hum': ('energy beam charging, rising electrical hum', 1.5),
 'beam_blast': ('powerful energy beam blast firing with a roar', 1.5),
 'explosion': ('big explosion with debris', 1.5),
 'impact_boom': ('massive ground impact boom with a rumble', 1.2),
 'cut_in': ('dramatic cut-in sting, sharp swoosh with a metallic shing', 1.0),
 'domain_drone': ('dark mystical domain expanding, deep resonant drone with wind', 3.0),
 'weapon_pickup': ('picking up a magical weapon, bright sparkling chime', 0.8),
 'weapon_break': ('metal weapon shattering into pieces', 1.0),
 'reflect': ('projectile reflected, bright ping with energy', 0.6),
}
VOICES = {
 'title': 'Super Clash Bros!', 'choose': 'Choose your fighter!', 'stage': 'Choose your stage!',
 'three': 'Three!', 'two': 'Two!', 'one': 'One!', 'go': 'GO!', 'game': 'GAME!', 'winner': 'The winner is...',
 'random': 'Random!', 'superclash': 'SUPER CLASH!', 'training': 'Training!',
 'asta': 'Asta!', 'yuno': 'Yuno!', 'ichigo': 'Ichigo!', 'byakuya': 'Byakuya!', 'kenpachi': 'Kenpachi!',
 'ulquiorra': 'Ulquiorra!', 'grimmjow': 'Grimmjow!', 'starrk': 'Stark!', 'naruto': 'Naruto!', 'sasuke': 'Sasuke!',
 'kakashi': 'Kakashi!', 'gojo': 'Gojo!', 'sukuna': 'Sukuna!', 'link': 'Link!', 'ganondorf': 'Ganondorf!',
 'kratos': 'Kratos!', 'hunter': 'Hunter!',
}
def post(url, body, path):
    r = subprocess.run(['curl', '-sS', '--max-time', '180', '-X', 'POST', url, '-H', 'xi-api-key: ' + KEY,
                        '-H', 'Content-Type: application/json', '-d', json.dumps(body), '-o', path, '-w', '%{http_code}'],
                       capture_output=True, text=True)
    return r.stdout
os.makedirs(out, exist_ok=True)
for name, (text, dur) in SFX.items():
    p = os.path.join(out, f'sfx_{name}.mp3')
    if os.path.exists(p): continue
    code = post('https://api.elevenlabs.io/v1/sound-generation', {'text': S + text, 'duration_seconds': dur, 'prompt_influence': 0.6}, p)
    print('sfx', name, code, flush=True)
for name, text in VOICES.items():
    p = os.path.join(out, f'voice_{name}.mp3')
    if os.path.exists(p): continue
    code = post(f'https://api.elevenlabs.io/v1/text-to-speech/{VOICE}?output_format=mp3_44100_128',
                {'text': text, 'model_id': 'eleven_multilingual_v2',
                 'voice_settings': {'stability': 0.35, 'similarity_boost': 0.8, 'style': 0.7}}, p)
    print('voice', name, code, flush=True)
