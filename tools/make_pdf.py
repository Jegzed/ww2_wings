"""Builds docs/WW2_Wings.pdf from the screenshots in shots/pdf (needs reportlab and Pillow)."""
import os
from PIL import Image
from reportlab.lib.colors import HexColor, Color
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas
from reportlab.platypus import Paragraph, Table, TableStyle

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHOTS = os.path.join(ROOT, 'shots', 'pdf')
TMP = os.path.join(ROOT, 'shots', 'pdfimg')
OUT = os.path.join(ROOT, 'docs', 'WW2_Wings.pdf')
os.makedirs(TMP, exist_ok=True)
os.makedirs(os.path.dirname(OUT), exist_ok=True)

W, H = landscape(A4)
M = 46  # margin

PAPER = HexColor('#F3EEDF')
INK = HexColor('#1E2026')
MUTED = HexColor('#5B5D62')
AMBER = HexColor('#C98A1B')
OLIVE = HexColor('#2B3121')
DARK = HexColor('#101310')
CREAM = HexColor('#EFE9D6')
RULE = HexColor('#CFC7B0')


def font(name, files, fallback):
    for f in files:
        p = os.path.join(r'C:\Windows\Fonts', f)
        if os.path.exists(p):
            try:
                pdfmetrics.registerFont(TTFont(name, p))
                return name
            except Exception:
                pass
    return fallback


HEAD = font('Head', ['bahnschrift.ttf', 'arialbd.ttf'], 'Helvetica-Bold')
BODY = font('Body', ['georgia.ttf', 'arial.ttf'], 'Helvetica')
BODY_B = font('BodyB', ['georgiab.ttf', 'arialbd.ttf'], 'Helvetica-Bold')
BODY_I = font('BodyI', ['georgiai.ttf', 'ariali.ttf'], 'Helvetica-Oblique')
MONO = font('Mono', ['consola.ttf', 'cour.ttf'], 'Courier')
if BODY == 'Body' and BODY_B == 'BodyB' and BODY_I == 'BodyI':
    from reportlab.pdfbase.pdfmetrics import registerFontFamily
    registerFontFamily('Body', normal='Body', bold='BodyB', italic='BodyI', boldItalic='BodyB')

P_BODY = ParagraphStyle('body', fontName=BODY, fontSize=11, leading=16.5, textColor=INK, alignment=TA_LEFT)
P_SMALL = ParagraphStyle('small', parent=P_BODY, fontSize=9.5, leading=14, textColor=MUTED)
P_LEAD = ParagraphStyle('lead', parent=P_BODY, fontSize=13.5, leading=20)
P_BULLET = ParagraphStyle('bullet', parent=P_BODY, leftIndent=14, bulletIndent=0, spaceAfter=5)
P_CELL = ParagraphStyle('cell', parent=P_BODY, fontSize=10, leading=14)
P_CELL_B = ParagraphStyle('cellb', parent=P_CELL, fontName=BODY_B)
P_CODE = ParagraphStyle('code', parent=P_BODY, fontName=MONO, fontSize=10, leading=14)


def jpg(name, crop=None, max_w=1600):
    """Converts a screenshot to JPEG (optionally cropped) and returns path and aspect."""
    src = os.path.join(SHOTS, name)
    im = Image.open(src).convert('RGB')
    if crop:
        im = im.crop(crop)
    if im.width > max_w:
        im = im.resize((max_w, int(im.height * max_w / im.width)), Image.LANCZOS)
    key = name.replace('.png', '') + ('_c%d_%d_%d_%d' % crop if crop else '')
    dst = os.path.join(TMP, key + '.jpg')
    im.save(dst, quality=88)
    return dst, im.width / im.height


def image(c, name, x, y, w, crop=None, frame=True):
    """Draws an image with its top-left at (x, y); returns its height."""
    path, aspect = jpg(name, crop)
    h = w / aspect
    c.drawImage(path, x, y - h, w, h)
    if frame:
        c.setStrokeColor(HexColor('#00000033', hasAlpha=True))
        c.setLineWidth(0.6)
        c.rect(x, y - h, w, h, stroke=1, fill=0)
    return h


def para(c, text, x, y, w, style=P_BODY):
    """Draws a paragraph with its top at y; returns the new y below it."""
    p = Paragraph(text, style)
    _, h = p.wrapOn(c, w, 1000)
    p.drawOn(c, x, y - h)
    return y - h


def bullets(c, items, x, y, w, gap=0):
    for it in items:
        p = Paragraph(it, P_BULLET, bulletText='\u2022')
        _, h = p.wrapOn(c, w, 1000)
        p.drawOn(c, x, y - h)
        y -= h + P_BULLET.spaceAfter + gap
    return y


def caption(c, text, x, y, w):
    return para(c, text, x, y - 5, w, P_SMALL)


class Doc:
    def __init__(self):
        self.c = canvas.Canvas(OUT, pagesize=(W, H))
        self.c.setTitle('WW2 Wings - Game Overview')
        self.c.setAuthor('Jegzed')
        self.c.setSubject('Overview of the WW2 Wings vertical slice')
        self.page = 0

    def start(self, kicker, title):
        c = self.c
        self.page += 1
        c.setFillColor(PAPER)
        c.rect(0, 0, W, H, stroke=0, fill=1)
        # header
        c.setFillColor(AMBER)
        c.setFont(HEAD, 10)
        c.drawString(M, H - 44, kicker.upper())
        c.setFillColor(INK)
        c.setFont(HEAD, 30)
        c.drawString(M, H - 78, title)
        c.setStrokeColor(AMBER)
        c.setLineWidth(2)
        c.line(M, H - 90, M + 64, H - 90)
        # footer
        c.setStrokeColor(RULE)
        c.setLineWidth(0.6)
        c.line(M, 34, W - M, 34)
        c.setFillColor(MUTED)
        c.setFont(HEAD, 8.5)
        c.drawString(M, 21, 'WW2 WINGS   \u00B7   GAME OVERVIEW')
        c.drawRightString(W - M, 21, str(self.page))
        return H - 112  # top of content

    def end(self):
        self.c.showPage()

    def save(self):
        self.c.save()


doc = Doc()
c = doc.c

# ---------------------------------------------------------------- cover
doc.page += 1
# The darkening gradient is baked into the picture so it renders without seams.
_im = Image.open(os.path.join(SHOTS, 'viewer_0.png')).convert('RGB')
_tw = int(_im.height * W / H)
_x0 = (_im.width - _tw) // 2
_im = _im.crop((_x0, 0, _x0 + _tw, _im.height))
_px = _im.load()
for _y in range(_im.height):
    _b = max(0.0, 1.0 - (_im.height - _y) / (_im.height * 0.3)) ** 2 * 0.6
    for _x in range(_im.width):
        _a = 0.8 * max(0.0, 1.0 - _x / (_im.width * 0.78)) ** 1.5
        _k = (1.0 - _a) * (1.0 - _b)
        r, g, b = _px[_x, _y]
        _px[_x, _y] = (int(r * _k + 8 * (1 - _k)), int(g * _k + 11 * (1 - _k)), int(b * _k + 8 * (1 - _k)))
_cover = os.path.join(TMP, 'cover.jpg')
_im.save(_cover, quality=90)
c.drawImage(_cover, 0, 0, W, H)
c.setFillColor(CREAM)
c.setFont(HEAD, 84)
c.drawString(M + 6, H - 190, 'WW2 WINGS')
c.setStrokeColor(AMBER)
c.setLineWidth(3)
c.line(M + 10, H - 212, M + 330, H - 212)
c.setFillColor(HexColor('#F0B94A'))
c.setFont(HEAD, 19)
c.drawString(M + 10, H - 244, 'N O R M A N D Y   \u00B7   1 9 4 4')
c.setFillColor(CREAM)
c.setFont(BODY_I, 14)
c.drawString(M + 10, H - 300, "A pilot's career in three mini-games.")
c.drawString(M + 10, H - 320, "A tribute to Cinemaware's Wings (1990).")
c.setFont(HEAD, 10.5)
c.setFillColor(Color(1, 1, 1, alpha=0.75))
c.drawString(M + 10, 46, 'GAME OVERVIEW   \u00B7   VERTICAL SLICE   \u00B7   SEPTEMBER 2026')
c.showPage()

# ---------------------------------------------------------------- overview
y = doc.start('The game', 'A career, one sortie at a time')
colw = 330
yy = para(c, "You are a replacement pilot posted to the 406th Fighter Squadron in the first days of June 1944. "
             "You fly the P-47 Thunderbolt, and the invasion of France is about to begin.", M, y, colw, P_LEAD)
yy = para(c, "WW2 Wings follows the shape of Cinemaware's 1990 Amiga classic <i>Wings</i>, moved from the Great War "
             "to the Second. The story is told through your pilot's diary. Between entries you fly, and every "
             "sortie is one of three different mini-games.", M, yy - 12, colw)
yy = bullets(c, [
    "<b>Air combat</b> in full 3D, seen from behind your aircraft.",
    "<b>Bombing</b> from directly overhead.",
    "<b>Ground attack</b> at tree-top height, seen from an isometric camera.",
], M, yy - 12, colw)
yy = para(c, "Around the flying sits the career: a pilot you create, aptitudes that grow with experience, "
             "promotions and medals, and the real chance that he does not come home. When that happens a "
             "replacement takes his bunk and the war goes on.", M, yy - 6, colw)
ix = M + colw + 30
iw = W - M - ix
ih = image(c, 'menu_0.png', ix, y, iw)
caption(c, 'The main menu. The flight of Thunderbolts behind it is rendered live.', ix, y - ih, iw)

# fact strip
fy = 108
facts = [('3', 'mini-games'), ('18', 'mission variants'), ('18', 'campaign missions'), ('4', 'pilot aptitudes'), ('0', 'art or sound files')]
fw = (W - 2 * M) / len(facts)
c.setStrokeColor(RULE)
c.setLineWidth(0.6)
c.line(M, fy + 44, W - M, fy + 44)
for i, (n, label) in enumerate(facts):
    x = M + i * fw
    c.setFillColor(AMBER)
    c.setFont(HEAD, 30)
    c.drawString(x, fy + 6, n)
    c.setFillColor(MUTED)
    c.setFont(HEAD, 10)
    c.drawString(x, fy - 10, label.upper())
doc.end()


# ---------------------------------------------------------------- mini-game pages
def minigame(kicker, title, lead, points, main, main_cap, side, side_cap, wings_note):
    y = doc.start(kicker, title)
    iw = 470
    ih = image(c, main, M, y, iw)
    caption(c, main_cap, M, y - ih, iw)
    # secondary image under the main one
    sy = y - ih - 34
    sw = 226
    sh = image(c, side, M, sy, sw)
    para(c, side_cap, M + sw + 14, sy - 2, iw - sw - 14, P_SMALL)

    tx = M + iw + 28
    tw = W - M - tx
    yy = para(c, lead, tx, y, tw, P_LEAD)
    yy = bullets(c, points, tx, yy - 12, tw)
    # callout
    p = Paragraph(wings_note, P_SMALL)
    _, ph = p.wrapOn(c, tw - 24, 1000)
    by = 52
    c.setFillColor(HexColor('#E8E0C8'))
    c.rect(tx, by, tw, ph + 34, stroke=0, fill=1)
    c.setFillColor(AMBER)
    c.rect(tx, by, 3, ph + 34, stroke=0, fill=1)
    c.setFont(HEAD, 9)
    c.drawString(tx + 14, by + ph + 17, 'IN THE ORIGINAL')
    p.drawOn(c, tx + 14, by + 9)
    doc.end()


minigame('Mini-game one', 'Air combat',
         "Dogfights against Bf 109s and Fw 190s, flown from a chase camera with your wingmen beside you.",
         [
             "Roll toward the enemy, then pull. Banked wings carry the nose around, and the aircraft eases "
             "back to level when you let go of the stick.",
             "A <b>lead marker</b> shows where to shoot; brackets, range and edge arrows keep track of every bandit.",
             "Damaged aircraft trail smoke, handle worse and finally go down burning.",
             "Ammunition is limited. Run dry and you break off for home.",
             "<b>Six variants:</b> fighter sweep, escorting B-26 Marauders, intercepting Ju 88s with tail gunners, "
             "chasing V-1 flying bombs, being bounced from behind, and a duel with an ace.",
         ],
         'dog_3.png', 'Escort duty: a box of B-26 Marauders with the Thunderbolts above them.',
         'dog_2.png', 'Contacts are bracketed with their range; friendly aircraft are marked in blue. Distant aircraft are drawn larger than life.',
         "Wings showed its dogfights from behind the pilot's head. The idea is kept; the view is pulled back "
         "to a modern chase camera.")

minigame('Mini-game two', 'Bombing',
         "A run over the target from directly above, with a limited bomb load and flak coming up to meet you.",
         [
             "Bombs take time to fall. A <b>ring on the ground</b> shows where one released now will land.",
             "Rolling stock, warehouses and fuel dumps burn, and fuel sets off whatever stands next to it.",
             "Flak is aimed at where you are going to be. Keep moving.",
             "Miss the objective and you can come around for one more pass.",
             "<b>Six variants:</b> rail yard, bridge, a moving armoured column, a harbour full of barges, "
             "a V-1 launch site under heavy flak, and an airfield.",
         ],
         'bomb_3.png', 'A stick of bombs walks through the rail yard at Amiens.',
         'bomb_1.png', 'A goods train caught on the main line on the way in.',
         "The top-down bombing run is the closest of the three to the 1990 game. The impact marker and the "
         "second pass are additions.")

minigame('Mini-game three', 'Ground attack',
         "Strafing at tree-top height along a road or an airfield, seen from an isometric camera.",
         [
             "Slide left and right, climb and dive. Your rounds strike ahead of you, and <b>the lower you fly "
             "the tighter the pattern</b>.",
             "Low is also where the poplars, barns and church towers are. Hit one and you may not get a second chance.",
             "Your shadow on the ground is the best guide to your height.",
             "Trucks burn easily; tanks shrug off most of what you throw at them.",
             "<b>Six variants:</b> convoy, airfield, train busting, a coastal battery among bunkers and surf, "
             "barges on the Seine, and a headquarters in a village.",
         ],
         'field_0.png', 'The coastal battery: bunkers in the dunes, wrecked landing craft in the surf.',
         'strafe_1.png', 'A flak position beside the Caen road, about to be hit.',
         "Wings flew its strafing runs diagonally across an isometric landscape, with height as the second "
         "axis. That is unchanged.")

# ---------------------------------------------------------------- career
y = doc.start('Between sorties', 'The career')
gap = 14
iw = (W - 2 * M - 2 * gap) / 3
names = [('diary_0.png', 'The diary sets the scene before each mission.'),
         ('briefing_0.png', 'The briefing: orders, route map and controls.'),
         ('debrief_0.png', 'The combat report: results, promotion, medals.')]
ih = 0
for i, (n, cap) in enumerate(names):
    x = M + i * (iw + gap)
    ih = image(c, n, x, y, iw)
    caption(c, cap, x, y - ih, iw)
ty = y - ih - 40
colw = (W - 2 * M - 30) / 2
yy = para(c, "<b>Your pilot.</b> You name him, say where he is from and spend points on four aptitudes. "
             "A successful sortie raises the aptitude it exercised.", M, ty, colw)
rows = [
    [Paragraph('Flying', P_CELL_B), Paragraph('How tightly the aircraft turns and how quickly it answers.', P_CELL)],
    [Paragraph('Shooting', P_CELL_B), Paragraph('Tighter bursts and harder hits.', P_CELL)],
    [Paragraph('Mechanical', P_CELL_B), Paragraph('Less damage taken; more ammunition and bombs carried.', P_CELL)],
    [Paragraph('Stamina', P_CELL_B), Paragraph('The odds of surviving when you are shot down.', P_CELL)],
]
t = Table(rows, colWidths=[78, colw - 78])
t.setStyle(TableStyle([
    ('VALIGN', (0, 0), (-1, -1), 'TOP'),
    ('LINEBELOW', (0, 0), (-1, -2), 0.5, RULE),
    ('LEFTPADDING', (0, 0), (-1, -1), 0),
    ('TOPPADDING', (0, 0), (-1, -1), 4),
    ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
]))
_, th = t.wrapOn(c, colw, 1000)
t.drawOn(c, M, yy - 10 - th)

x2 = M + colw + 30
yy = para(c, "<b>Rank and decorations.</b> Score earns promotion from 2nd Lieutenant up to Lieutenant Colonel. "
             "Five decorations can be won: the Air Medal, the Distinguished Flying Cross for five aerial "
             "victories, the Silver Star for an outstanding sortie, the Purple Heart and the Distinguished "
             "Unit Citation.", x2, ty, colw)
yy = para(c, "<b>Going down.</b> Being shot down does not always end a career. Depending on his stamina a pilot "
             "may bail out and walk home, come back wounded, or be killed. A dead pilot is remembered on the "
             "squadron roster, and a replacement you create takes over the campaign where it stands.",
          x2, yy - 10, colw)
yy = para(c, "Progress is saved after every sortie.", x2, yy - 10, colw)
doc.end()

# ---------------------------------------------------------------- campaign
y = doc.start('The campaign', 'Seven weeks over Normandy')
y = para(c, "Eighteen missions from the eve of the invasion to the Seine, each a different variant. Any of the three "
            "mini-games can also be started from the menu as <b>Instant Action</b>, which picks a random variant, "
            "time of day and weather and does not touch your career.", M, y, W - 2 * M - 120)
P_ROW = ParagraphStyle('row', parent=P_CELL, fontSize=8.6, leading=10.5)
P_ROW_B = ParagraphStyle('rowb', parent=P_ROW, fontName=BODY_B)
head = ParagraphStyle('th', parent=P_ROW, fontName=HEAD, fontSize=8, textColor=MUTED)
missions = [
    ('June 2', 'Fighter Sweep', 'Air combat', 'Sweep', 'Pas-de-Calais', 1),
    ('June 4', 'The Rail Yard', 'Bombing', 'Rail yard', 'Amiens', 1),
    ('June 6', 'D-Day', 'Ground attack', 'Convoy', 'Road to Caen', 2),
    ('June 7', 'Little Friends', 'Air combat', 'Bomber escort', 'Caen', 2),
    ('June 9', 'The Battery', 'Ground attack', 'Coastal battery', 'Utah Beach', 2),
    ('June 10', 'Bandits over the Beachhead', 'Air combat', 'Sweep', 'Sainte-Mere-Eglise', 3),
    ('June 12', 'Train Busting', 'Ground attack', 'Train', 'Lisieux', 2),
    ('June 14', 'The Bridge', 'Bombing', 'Bridge', 'River Orne', 3),
    ('June 16', 'Bombers Inbound', 'Air combat', 'Intercept', 'The anchorage', 3),
    ('June 18', 'Airfield Attack', 'Ground attack', 'Airfield', 'Evreux', 4),
    ('June 20', 'Noball', 'Bombing', 'Launch site', 'Saint-Omer', 3),
    ('June 23', 'Diver Patrol', 'Air combat', 'V-1 patrol', 'Kent coast', 3),
    ('June 26', 'The Harbour', 'Bombing', 'Harbour', 'Le Havre', 3),
    ('June 30', 'Jumped', 'Air combat', 'Ambush', 'Falaise', 4),
    ('July 4', 'The Headquarters', 'Ground attack', 'Village HQ', 'Villers-Bocage', 3),
    ('July 8', 'Column on the Move', 'Bombing', 'Moving column', 'Saint-Lo', 3),
    ('July 12', 'The Red-Nosed 109', 'Air combat', 'Duel', 'Caen', 5),
    ('July 18', 'River Traffic', 'Ground attack', 'River barges', 'The Seine', 4),
]
half = 9
cols_w = [38, 92, 54, 62, 62, 50]
tw = sum(cols_w)
for col in range(2):
    data = [[Paragraph(h, head) for h in ['DATE', 'MISSION', 'TYPE', 'VARIANT', 'WHERE', 'DIFF.']]]
    for d, m, t_, v_, w_, k in missions[col * half:(col + 1) * half]:
        dots = '<font color="#C98A1B">' + '\u25A0' * k + '</font><font color="#CFC7B0">' + '\u25A0' * (5 - k) + '</font>'
        data.append([Paragraph(d, P_ROW), Paragraph(m, P_ROW_B), Paragraph(t_, P_ROW), Paragraph(v_, P_ROW),
                     Paragraph(w_, P_ROW), Paragraph(dots, ParagraphStyle('d', parent=P_ROW, fontName='Helvetica'))])
    t = Table(data, colWidths=cols_w)
    t.setStyle(TableStyle([
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
        ('LINEBELOW', (0, 0), (-1, 0), 1.0, INK),
        ('LINEBELOW', (0, 1), (-1, -1), 0.4, RULE),
        ('LEFTPADDING', (0, 0), (-1, -1), 0),
        ('RIGHTPADDING', (0, 0), (-1, -1), 4),
        ('TOPPADDING', (0, 0), (-1, -1), 5),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 5),
    ]))
    _, th = t.wrapOn(c, tw, 1000)
    x = M + col * (tw + 30)
    t.drawOn(c, x, y - 18 - th)
doc.end()

# ---------------------------------------------------------------- controls and running
y = doc.start('Playing', 'Controls and how to run it')
colw = (W - 2 * M - 36) / 2
head = ParagraphStyle('th2', parent=P_CELL, fontName=HEAD, fontSize=9, textColor=MUTED)
data = [[Paragraph(h, head) for h in ['ACTION', 'KEYBOARD', 'GAMEPAD']],
        ['Steer', 'W A S D or arrow keys', 'Left stick'],
        ['Guns and bombs', 'Space', 'A, right trigger'],
        ['Throttle (air combat)', 'Shift to open, Ctrl to close', 'Shoulder buttons'],
        ['Pause', 'Esc', 'Start'],
        ['Fullscreen', 'F11', '']]
data = [data[0]] + [[Paragraph(a, P_CELL_B), Paragraph(b, P_CELL), Paragraph(g, P_CELL)] for a, b, g in data[1:]]
t = Table(data, colWidths=[130, colw - 130 - 100, 100])
t.setStyle(TableStyle([
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LINEBELOW', (0, 0), (-1, 0), 1.2, INK),
    ('LINEBELOW', (0, 1), (-1, -1), 0.5, RULE),
    ('LEFTPADDING', (0, 0), (-1, -1), 0),
    ('TOPPADDING', (0, 0), (-1, -1), 7),
    ('BOTTOMPADDING', (0, 0), (-1, -1), 7),
]))
_, th = t.wrapOn(c, colw, 1000)
t.drawOn(c, M, y - th)
yy = y - th - 18
yy = bullets(c, [
    "<b>Air combat:</b> S pulls the nose up, W pushes it down, A and D roll.",
    "<b>Bombing:</b> steer in all four directions over the scrolling ground.",
    "<b>Ground attack:</b> A and D slide sideways, W dives, S climbs.",
], M, yy, colw)

x2 = M + colw + 36
yy = para(c, "<b>The exported build</b>", x2, y, colw)
yy = para(c, "A Windows build is in the <font name='%s'>export</font> folder of the project. Keep the two files together; "
             "the game will not start without the DLL beside it." % MONO, x2, yy - 4, colw)
yy = para(c, "export\\WW2Wings.exe<br/>export\\ww2wings.dll", x2, yy - 8, colw, P_CODE)
yy = para(c, "<b>From source</b>", x2, yy - 18, colw)
yy = para(c, "Requires Windows, Godot 4.7, Visual Studio 2022 with the C++ workload and CMake.", x2, yy - 4, colw)
yy = para(c, "git submodule update --init<br/>build.bat<br/>run.bat", x2, yy - 8, colw, P_CODE)
yy = para(c, "The first build compiles the Godot C++ bindings and takes a few minutes.", x2, yy - 8, colw, P_SMALL)
doc.end()

# ---------------------------------------------------------------- under the hood
y = doc.start('Under the hood', 'Everything is made in code')
iw = 360
ih = image(c, 'viewer_1.png', W - M - iw, y, iw)
caption(c, 'The model viewer: every aircraft, vehicle and building is generated at start-up.', W - M - iw, y - ih, iw)
tw = W - 2 * M - iw - 30
yy = para(c, "The game is written in C++ as a Godot 4.7 GDExtension, in roughly 10,900 lines. The Godot project "
             "itself is a single scene with a single node. The repository contains no models, textures or audio.",
          M, y, tw, P_LEAD)
yy = bullets(c, [
    "<b>Aircraft</b> are lofted from cross-sections and aerofoil profiles. Paint, camouflage, invasion stripes, "
    "national markings and panel lines are drawn by a shader.",
    "<b>The landscape</b> is a shader too: the irregular fields and hedgerows of Normandy, rivers, roads, "
    "railways and runways.",
    "<b>Sound</b> is synthesised when the game starts: engines, guns, explosions, the falling-bomb whistle and the music.",
    "<b>Lighting</b> uses Godot's Forward+ renderer with a procedural sky, shadows, ambient occlusion, fog and bloom.",
], M, yy - 12, tw)

# status box across the bottom
bx, bw = M, W - 2 * M
by, bh = 50, 132
c.setFillColor(OLIVE)
c.rect(bx, by, bw, bh, stroke=0, fill=1)
c.setFillColor(HexColor('#F0B94A'))
c.setFont(HEAD, 10)
c.drawString(bx + 20, by + bh - 24, 'STATUS OF THE VERTICAL SLICE')
light = ParagraphStyle('light', parent=P_BODY, textColor=CREAM, fontSize=10.5, leading=15.5)
light_b = ParagraphStyle('lightb', parent=light, leftIndent=12, bulletIndent=0, spaceAfter=3)
cw = (bw - 40 - 30) / 2
p1 = [
    "The full loop plays from menu to ending, and has been run unattended from start to finish.",
    "Player controls and the pause menu were checked with simulated key presses.",
]
p2 = [
    "Difficulty has only been tuned against a simple autopilot. <b>It has had no structured play-testing.</b>",
    "Sound was checked numerically, not by ear.",
]
for col, items in enumerate([p1, p2]):
    yy = by + bh - 38
    for it in items:
        p = Paragraph(it, light_b, bulletText='\u2022')
        _, h = p.wrapOn(c, cw, 1000)
        p.drawOn(c, bx + 20 + col * (cw + 30), yy - h)
        yy -= h + 5
doc.end()

doc.save()
print('wrote', OUT, os.path.getsize(OUT) // 1024, 'KB', doc.page, 'pages')
