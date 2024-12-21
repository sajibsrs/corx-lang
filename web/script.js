const regex = hljs.regex;
const CORN_KEYWORDS = [
    'asm',
    'auto',
    'break',
    'case',
    'continue',
    'default',
    'do',
    'else',
    'enum',
    'extern',
    'for',
    'goto',
    'if',
    'interface',
    'inline',
    'register',
    'restrict',
    'return',
    'sizeof',
    'struct',
    'switch',
    'this',
    'typeof',
    'type',
    'union',
    'volatile',
    'while',
];

const CORN_TYPES = [
    'char',
    'float',
    'int',
    'long',
    'short',
    'signed',
    'unsigned',
    'void',
    // modifiers
    'const',
    'static',
    'complex',
    'bool',
    'imaginary',
];

const KEYWORDS = {
    keyword: CORN_KEYWORDS,
    type: CORN_TYPES,
    literal: 'true false null'
};

hljs.registerLanguage('corn', function (hljs) {
    return {
        case_insensitive: true, // language is case-insensitive
        keywords: KEYWORDS,
        disableAutodetect: true,
        contains: [
            hljs.C_LINE_COMMENT_MODE,
            hljs.C_BLOCK_COMMENT_MODE,
            hljs.C_NUMBER_MODE,
            hljs.HASH_COMMENT_MODE,
            hljs.APOS_STRING_MODE,
            hljs.QUOTE_STRING_MODE,
            // {
            //     scope: 'aggregate',
            //     beginKeywords: 'enum interface struct union',
            //     end: /[{;]/,
            //     contains: [
            //         {
            //             scope: 'type',
            //             begin: /\b[A-Za-z_]\w*\b/, // Matches type names like `vec2`
            //             relevance: 0,
            //         },
            //         // hljs.TITLE_MODE
            //     ]
            // },
            // {
            //     scope: 'function',
            //     begin: /(\s+|,)[a-zA-Z0-9_\.*]+\(/,
            //     end: /\)/,
            //     relevance: 0,
            //     contains: [
            //         hljs.C_NUMBER_MODE,
            //         {
            //             scope: 'type',
            //             begin: /\b[A-Za-z_]\w*\b/, // Matches type names like `vec2`
            //             relevance: 0,
            //         }
            //     ]
            // },
        ],
    };
});

window.onload = function () {
    hljs.highlightAll();
};
