/**
 * Convert an ascii value to a FabGL VirtualKey enum.
 *
 * Will return VK_NONE (zero) for ascii outside the range [32..127]
 */
pub fn ascii2vk(chr: char) -> u32 {
    (match chr {
        '\x08' => VirtualKey::VK_BACKSPACE,
        '\r' => VirtualKey::VK_RETURN,
        '\x1b' => VirtualKey::VK_ESCAPE,
        ' ' => VirtualKey::VK_SPACE,
        '!' => VirtualKey::VK_EXCLAIM,
        '"' => VirtualKey::VK_QUOTEDBL,
        '#' => VirtualKey::VK_HASH,
        '$' => VirtualKey::VK_DOLLAR,
        '%' => VirtualKey::VK_PERCENT,
        '&' => VirtualKey::VK_AMPERSAND,
        '\'' => VirtualKey::VK_QUOTE,
        '(' => VirtualKey::VK_LEFTPAREN,
        ')' => VirtualKey::VK_RIGHTPAREN,
        '*' => VirtualKey::VK_ASTERISK,
        '+' => VirtualKey::VK_PLUS,
        ',' => VirtualKey::VK_COMMA,
        '-' => VirtualKey::VK_MINUS,
        '.' => VirtualKey::VK_PERIOD,
        '/' => VirtualKey::VK_SLASH,
        '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' => unsafe {
            std::mem::transmute(VirtualKey::VK_0 as u32 + (chr as u32 - 48))
        },
        ':' => VirtualKey::VK_COLON,
        ';' => VirtualKey::VK_SEMICOLON,
        '<' => VirtualKey::VK_LESS,
        '=' => VirtualKey::VK_EQUALS,
        '>' => VirtualKey::VK_GREATER,
        '?' => VirtualKey::VK_QUESTION,
        '@' => VirtualKey::VK_AT,
        'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G' | 'H' | 'I' | 'J' | 'K' | 'L' | 'M' | 'N' | 'O'
        | 'P' | 'Q' | 'R' | 'S' | 'T' | 'U' | 'V' | 'W' | 'X' | 'Y' | 'Z' => unsafe {
            std::mem::transmute(VirtualKey::VK_A as u32 + (chr as u32 - 65))
        },
        '[' => VirtualKey::VK_LEFTBRACKET,
        '^' => VirtualKey::VK_ACUTEACCENT,
        '_' => VirtualKey::VK_UNDERSCORE,
        '`' => VirtualKey::VK_GRAVEACCENT,
        'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i' | 'j' | 'k' | 'l' | 'm' | 'n' | 'o'
        | 'p' | 'q' | 'r' | 's' | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z' => unsafe {
            std::mem::transmute(VirtualKey::VK_a as u32 + (chr as u32 - 97))
        },
        '{' => VirtualKey::VK_LEFTBRACE,
        '|' => VirtualKey::VK_VERTICALBAR,
        '}' => VirtualKey::VK_RIGHTBRACE,
        '~' => VirtualKey::VK_TILDE,
        '\x7f' => VirtualKey::VK_BACKSPACE, // XXX is this right?
        _ => VirtualKey::VK_NONE,
    }) as u32
}

/* From FabGL */
#[repr(u32)]
#[allow(non_snake_case, non_camel_case_types)]
enum VirtualKey {
    VK_NONE,
    /**< No character (marks the first virtual key) */
    VK_SPACE,
    /**< Space */
    VK_0,
    /**< Number 0 */
    VK_1,
    /**< Number 1 */
    VK_2,
    /**< Number 2 */
    VK_3,
    /**< Number 3 */
    VK_4,
    /**< Number 4 */
    VK_5,
    /**< Number 5 */
    VK_6,
    /**< Number 6 */
    VK_7,
    /**< Number 7 */
    VK_8,
    /**< Number 8 */
    VK_9,
    /**< Number 9 */
    VK_KP_0,
    /**< Keypad number 0 */
    VK_KP_1,
    /**< Keypad number 1 */
    VK_KP_2,
    /**< Keypad number 2 */
    VK_KP_3,
    /**< Keypad number 3 */
    VK_KP_4,
    /**< Keypad number 4 */
    VK_KP_5,
    /**< Keypad number 5 */
    VK_KP_6,
    /**< Keypad number 6 */
    VK_KP_7,
    /**< Keypad number 7 */
    VK_KP_8,
    /**< Keypad number 8 */
    VK_KP_9,
    /**< Keypad number 9 */
    VK_a,
    /**< Lower case letter 'a' */
    VK_b,
    /**< Lower case letter 'b' */
    VK_c,
    /**< Lower case letter 'c' */
    VK_d,
    /**< Lower case letter 'd' */
    VK_e,
    /**< Lower case letter 'e' */
    VK_f,
    /**< Lower case letter 'f' */
    VK_g,
    /**< Lower case letter 'g' */
    VK_h,
    /**< Lower case letter 'h' */
    VK_i,
    /**< Lower case letter 'i' */
    VK_j,
    /**< Lower case letter 'j' */
    VK_k,
    /**< Lower case letter 'k' */
    VK_l,
    /**< Lower case letter 'l' */
    VK_m,
    /**< Lower case letter 'm' */
    VK_n,
    /**< Lower case letter 'n' */
    VK_o,
    /**< Lower case letter 'o' */
    VK_p,
    /**< Lower case letter 'p' */
    VK_q,
    /**< Lower case letter 'q' */
    VK_r,
    /**< Lower case letter 'r' */
    VK_s,
    /**< Lower case letter 's' */
    VK_t,
    /**< Lower case letter 't' */
    VK_u,
    /**< Lower case letter 'u' */
    VK_v,
    /**< Lower case letter 'v' */
    VK_w,
    /**< Lower case letter 'w' */
    VK_x,
    /**< Lower case letter 'x' */
    VK_y,
    /**< Lower case letter 'y' */
    VK_z,
    /**< Lower case letter 'z' */
    VK_A,
    /**< Upper case letter 'A' */
    VK_B,
    /**< Upper case letter 'B' */
    VK_C,
    /**< Upper case letter 'C' */
    VK_D,
    /**< Upper case letter 'D' */
    VK_E,
    /**< Upper case letter 'E' */
    VK_F,
    /**< Upper case letter 'F' */
    VK_G,
    /**< Upper case letter 'G' */
    VK_H,
    /**< Upper case letter 'H' */
    VK_I,
    /**< Upper case letter 'I' */
    VK_J,
    /**< Upper case letter 'J' */
    VK_K,
    /**< Upper case letter 'K' */
    VK_L,
    /**< Upper case letter 'L' */
    VK_M,
    /**< Upper case letter 'M' */
    VK_N,
    /**< Upper case letter 'N' */
    VK_O,
    /**< Upper case letter 'O' */
    VK_P,
    /**< Upper case letter 'P' */
    VK_Q,
    /**< Upper case letter 'Q' */
    VK_R,
    /**< Upper case letter 'R' */
    VK_S,
    /**< Upper case letter 'S' */
    VK_T,
    /**< Upper case letter 'T' */
    VK_U,
    /**< Upper case letter 'U' */
    VK_V,
    /**< Upper case letter 'V' */
    VK_W,
    /**< Upper case letter 'W' */
    VK_X,
    /**< Upper case letter 'X' */
    VK_Y,
    /**< Upper case letter 'Y' */
    VK_Z,
    /**< Upper case letter 'Z' */
    VK_GRAVEACCENT,
    /**< Grave accent: ` */
    VK_ACUTEACCENT,
    /**< Acute accent: ´ */
    VK_QUOTE,
    /**< Quote: ' */
    VK_QUOTEDBL,
    /**< Double quote: " */
    VK_EQUALS,
    /**< Equals: = */
    VK_MINUS,
    /**< Minus: - */
    VK_KP_MINUS,
    /**< Keypad minus: - */
    VK_PLUS,
    /**< Plus: + */
    VK_KP_PLUS,
    /**< Keypad plus: + */
    VK_KP_MULTIPLY,
    /**< Keypad multiply: * */
    VK_ASTERISK,
    /**< Asterisk: * */
    VK_BACKSLASH,
    /**< Backslash: \ */
    VK_KP_DIVIDE,
    /**< Keypad divide: / */
    VK_SLASH,
    /**< Slash: / */
    VK_KP_PERIOD,
    /**< Keypad period: . */
    VK_PERIOD,
    /**< Period: . */
    VK_COLON,
    /**< Colon: : */
    VK_COMMA,
    /**< Comma: , */
    VK_SEMICOLON,
    /**< Semicolon: ; */
    VK_AMPERSAND,
    /**< Ampersand: & */
    VK_VERTICALBAR,
    /**< Vertical bar: | */
    VK_HASH,
    /**< Hash: # */
    VK_AT,
    /**< At: @ */
    VK_CARET,
    /**< Caret: ^ */
    VK_DOLLAR,
    /**< Dollar: $ */
    VK_POUND,
    /**< Pound: £ */
    VK_EURO,
    /**< Euro: € */
    VK_PERCENT,
    /**< Percent: % */
    VK_EXCLAIM,
    /**< Exclamation mark: ! */
    VK_QUESTION,
    /**< Question mark: ? */
    VK_LEFTBRACE,
    /**< Left brace: { */
    VK_RIGHTBRACE,
    /**< Right brace: } */
    VK_LEFTBRACKET,
    /**< Left bracket: [ */
    VK_RIGHTBRACKET,
    /**< Right bracket: ] */
    VK_LEFTPAREN,
    /**< Left parenthesis: ( */
    VK_RIGHTPAREN,
    /**< Right parenthesis: ) */
    VK_LESS,
    /**< Less: < */
    VK_GREATER,
    /**< Greater: > */
    VK_UNDERSCORE,
    /**< Underscore: _ */
    VK_DEGREE,
    /**< Degree: ° */
    VK_SECTION,
    /**< Section: § */
    VK_TILDE,
    /**< Tilde: ~ */
    VK_NEGATION,
    /**< Negation: ¬ */
    VK_LSHIFT,
    /**< Left SHIFT */
    VK_RSHIFT,
    /**< Right SHIFT */
    VK_LALT,
    /**< Left ALT */
    VK_RALT,
    /**< Right ALT */
    VK_LCTRL,
    /**< Left CTRL */
    VK_RCTRL,
    /**< Right CTRL */
    VK_LGUI,
    /**< Left GUI */
    VK_RGUI,
    /**< Right GUI */
    VK_ESCAPE,
    /**< ESC */
    VK_PRINTSCREEN,
    /**< PRINTSCREEN */
    VK_SYSREQ,
    /**< SYSREQ */
    VK_INSERT,
    /**< INS */
    VK_KP_INSERT,
    /**< Keypad INS */
    VK_DELETE,
    /**< DEL */
    VK_KP_DELETE,
    /**< Keypad DEL */
    VK_BACKSPACE,
    /**< Backspace */
    VK_HOME,
    /**< HOME */
    VK_KP_HOME,
    /**< Keypad HOME */
    VK_END,
    /**< END */
    VK_KP_END,
    /**< Keypad END */
    VK_PAUSE,
    /**< PAUSE */
    VK_BREAK,
    /**< CTRL + PAUSE */
    VK_SCROLLLOCK,
    /**< SCROLLLOCK */
    VK_NUMLOCK,
    /**< NUMLOCK */
    VK_CAPSLOCK,
    /**< CAPSLOCK */
    VK_TAB,
    /**< TAB */
    VK_RETURN,
    /**< RETURN */
    VK_KP_ENTER,
    /**< Keypad ENTER */
    VK_APPLICATION,
    /**< APPLICATION / MENU key */
    VK_PAGEUP,
    /**< PAGEUP */
    VK_KP_PAGEUP,
    /**< Keypad PAGEUP */
    VK_PAGEDOWN,
    /**< PAGEDOWN */
    VK_KP_PAGEDOWN,
    /**< Keypad PAGEDOWN */
    VK_UP,
    /**< Cursor UP */
    VK_KP_UP,
    /**< Keypad cursor UP  */
    VK_DOWN,
    /**< Cursor DOWN */
    VK_KP_DOWN,
    /**< Keypad cursor DOWN */
    VK_LEFT,
    /**< Cursor LEFT */
    VK_KP_LEFT,
    /**< Keypad cursor LEFT */
    VK_RIGHT,
    /**< Cursor RIGHT */
    VK_KP_RIGHT,
    /**< Keypad cursor RIGHT */
    VK_KP_CENTER,
    /**< Keypad CENTER key */
    VK_F1,
    /**< F1 function key */
    VK_F2,
    /**< F2 function key */
    VK_F3,
    /**< F3 function key */
    VK_F4,
    /**< F4 function key */
    VK_F5,
    /**< F5 function key */
    VK_F6,
    /**< F6 function key */
    VK_F7,
    /**< F7 function key */
    VK_F8,
    /**< F8 function key */
    VK_F9,
    /**< F9 function key */
    VK_F10,
    /**< F10 function key */
    VK_F11,
    /**< F11 function key */
    VK_F12,
    /**< F12 function key */
    VK_GRAVE_a,
    /**< Grave a: à */
    VK_GRAVE_e,
    /**< Grave e: è */
    VK_GRAVE_i,
    /**< Grave i: ì */
    VK_GRAVE_o,
    /**< Grave o: ò */
    VK_GRAVE_u,
    /**< Grave u: ù */
    VK_GRAVE_y,
    /**< Grave y: ỳ */
    VK_ACUTE_a,
    /**< Acute a: á */
    VK_ACUTE_e,
    /**< Acute e: é */
    VK_ACUTE_i,
    /**< Acute i: í */
    VK_ACUTE_o,
    /**< Acute o: ó */
    VK_ACUTE_u,
    /**< Acute u: ú */
    VK_ACUTE_y,
    /**< Acute y: ý */
    VK_GRAVE_A,
    /**< Grave A: À */
    VK_GRAVE_E,
    /**< Grave E: È */
    VK_GRAVE_I,
    /**< Grave I: Ì */
    VK_GRAVE_O,
    /**< Grave O: Ò */
    VK_GRAVE_U,
    /**< Grave U: Ù */
    VK_GRAVE_Y,
    /**< Grave Y: Ỳ */
    VK_ACUTE_A,
    /**< Acute A: Á */
    VK_ACUTE_E,
    /**< Acute E: É */
    VK_ACUTE_I,
    /**< Acute I: Í */
    VK_ACUTE_O,
    /**< Acute O: Ó */
    VK_ACUTE_U,
    /**< Acute U: Ú */
    VK_ACUTE_Y,
    /**< Acute Y: Ý */
    VK_UMLAUT_a,
    /**< Diaeresis a: ä */
    VK_UMLAUT_e,
    /**< Diaeresis e: ë */
    VK_UMLAUT_i,
    /**< Diaeresis i: ï */
    VK_UMLAUT_o,
    /**< Diaeresis o: ö */
    VK_UMLAUT_u,
    /**< Diaeresis u: ü */
    VK_UMLAUT_y,
    /**< Diaeresis y: ÿ */
    VK_UMLAUT_A,
    /**< Diaeresis A: Ä */
    VK_UMLAUT_E,
    /**< Diaeresis E: Ë */
    VK_UMLAUT_I,
    /**< Diaeresis I: Ï */
    VK_UMLAUT_O,
    /**< Diaeresis O: Ö */
    VK_UMLAUT_U,
    /**< Diaeresis U: Ü */
    VK_UMLAUT_Y,
    /**< Diaeresis Y: Ÿ */
    VK_CARET_a,
    /**< Caret a: â */
    VK_CARET_e,
    /**< Caret e: ê */
    VK_CARET_i,
    /**< Caret i: î */
    VK_CARET_o,
    /**< Caret o: ô */
    VK_CARET_u,
    /**< Caret u: û */
    VK_CARET_y,
    /**< Caret y: ŷ */
    VK_CARET_A,
    /**< Caret A: Â */
    VK_CARET_E,
    /**< Caret E: Ê */
    VK_CARET_I,
    /**< Caret I: Î */
    VK_CARET_O,
    /**< Caret O: Ô */
    VK_CARET_U,
    /**< Caret U: Û */
    VK_CARET_Y,
    /**< Caret Y: Ŷ */
    VK_CEDILLA_c,
    /**< Cedilla c: ç */
    VK_CEDILLA_C,
    /**< Cedilla C: Ç */
    VK_TILDE_a,
    /**< Lower case tilde a: ã */
    VK_TILDE_o,
    /**< Lower case tilde o: õ */
    VK_TILDE_n,
    /**< Lower case tilde n: ñ */
    VK_TILDE_A,
    /**< Upper case tilde A: Ã */
    VK_TILDE_O,
    /**< Upper case tilde O: Õ */
    VK_TILDE_N,
    /**< Upper case tilde N: Ñ */
    VK_UPPER_a,
    /**< primera: a */
    VK_ESZETT,
    /**< Eszett: ß */
    VK_EXCLAIM_INV,
    /**< Inverted exclamation mark: ! */
    VK_QUESTION_INV,
    /**< Inverted question mark : ? */
    VK_INTERPUNCT,
    /**< Interpunct : · */
    VK_DIAERESIS,
    /**< Diaeresis  : ¨ */
    VK_SQUARE,
    /**< Square     : ² */
    VK_CURRENCY,
    /**< Currency   : ¤ */
    VK_MU,
    /**< Mu         : µ */
    VK_HALF,
    /**< 1/2 fraction : 1/2 */
    VK_MASCULIN_ORD,
    /**< Masculin ordinal superscript o */
    VK_FEMININ_ORD,
    /**< Feminin ordinal superscript a */
    VK_LEFTGUILLEMET,
    /**< Left guillemet: « */
    VK_RIGHTGUILLEMET,
    /**< Right guillemet: » */
    VK_aelig,
    /** Lower case aelig  : æ */
    VK_oslash,
    /** Lower case oslash : ø */
    VK_aring,
    /** Lower case aring  : å */
    VK_AELIG,
    /** Upper case aelig  : Æ */
    VK_OSLASH,
    /** Upper case oslash : Ø */
    VK_ARING,
    /** Upper case aring  : Å */
    VK_DEAD_GRAVEACCENT,
    /** Grave accent when we need seperate code for dead key*/
    VK_DEAD_CARET,
    /** Caret when we need seperate code for dead key*/
    VK_DEAD_TILDE,
    /** Tilde when we need seperate code for dead key*/
    // Missing code page 1252 virtual keys
    VK_UPPER_1,
    /** Superscript 1   : ¹ */
    VK_CUBE,
    /** Superscript 3   : ³ */
    VK_CENT,
    /** Cent (currency) : ¢ */
    // Japanese layout support
    VK_YEN,
    VK_MUHENKAN,
    VK_HENKAN,
    VK_KATAKANA_HIRAGANA_ROMAJI,
    VK_HANKAKU_ZENKAKU_KANJI,
    VK_SHIFT_0,

    VK_ASCII, /*< Specifies an ASCII code - used when virtual key is embedded in VirtualKeyItem structure and VirtualKeyItem.ASCII is valid */
}
