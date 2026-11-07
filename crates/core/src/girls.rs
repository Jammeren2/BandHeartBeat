//! Anime girl badges: PNG art (512x512, transparent) holding an empty plate
//! where the BPM number is drawn.

use crate::settings::OverlayStyle;

pub struct Girl {
    pub png: &'static [u8],
    /// Centre of the plate as a fraction of the image.
    pub plate: (f32, f32),
    /// Number height as a fraction of the image.
    pub size: f32,
    /// Number colour (RGB) and whether it needs a shadow to stand out.
    pub color: (u8, u8, u8),
    pub shadow: bool,
}

const WHITE: (u8, u8, u8) = (255, 255, 255);

pub fn girl(style: OverlayStyle) -> Option<Girl> {
    use OverlayStyle::*;
    let (png, plate, size, color, shadow): (&'static [u8], _, _, _, _) = match style {
        GirlFox => (include_bytes!("../assets/girls/fox.png"), (0.50, 0.60), 0.17, WHITE, true),
        GirlMaid => (include_bytes!("../assets/girls/maid.png"), (0.50, 0.66), 0.14, (0x3A, 0x34, 0x48), false),
        GirlAngel => (include_bytes!("../assets/girls/angel.png"), (0.50, 0.67), 0.18, WHITE, true),
        GirlNurse => (include_bytes!("../assets/girls/nurse.png"), (0.49, 0.62), 0.15, (0xE0, 0x45, 0x7B), false),
        GirlWitch => (include_bytes!("../assets/girls/witch.png"), (0.50, 0.71), 0.16, WHITE, true),
        GirlSpace => (include_bytes!("../assets/girls/space.png"), (0.50, 0.58), 0.14, WHITE, true),
        GirlNeko => (include_bytes!("../assets/girls/neko.png"), (0.50, 0.70), 0.19, WHITE, true),
        GirlSakura => (include_bytes!("../assets/girls/sakura.png"), (0.50, 0.65), 0.15, (0x5A, 0x3A, 0x1E), false),
        GirlBunny => (include_bytes!("../assets/girls/bunny.png"), (0.50, 0.59), 0.16, WHITE, true),
        GirlSport => (include_bytes!("../assets/girls/sport.png"), (0.49, 0.61), 0.15, WHITE, true),
        _ => return None,
    };
    Some(Girl { png, plate, size, color, shadow })
}
