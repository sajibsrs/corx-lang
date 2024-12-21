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
    'for',
    'goto',
    'if',
    'interface',
    'inline',
    'register',
    'return',
    'sizeof',
    'switch',
    'this',
    'typeof',
    'type',
    'union',
    'volatile',
    'while',

    // encapsulation
    'external',
    'internal',
    'restrict',

    'private',
    'protect',
    'public',
];

const CORN_TYPES = [
    'char',
    'float',
    'int',
    'long',
    'short',
    'signed',
    'string',
    'unsigned',
    'void',
    'wchar',

    // abstract
    'enum',
    'struct',

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
            hljs.APOS_STRING_MODE,
            hljs.QUOTE_STRING_MODE,
            {
                scope: 'comment',
                variants: [
                    hljs.C_LINE_COMMENT_MODE,
                    hljs.C_BLOCK_COMMENT_MODE,
                    {
                        begin: /#/,
                        end: /\n/,
                        'on:begin': (m, n) => {
                            console.log(m, n);
                        },
                        contains: [
                            {
                                scope: 'doctag',
                                match: /@\w+/, ///@\w+/
                                'on:begin': (m, n) => {
                                    console.log(m, n);
                                },
                            },
                            {
                                scope: 'type',
                                begin: '\\<',
                                end: '\\>',
                                excludeBegin: true,
                                excludeEnd: true,
                            },
                            {
                                scope: 'keyword',
                                begin: '\\[',
                                end: '\\]',
                                excludeBegin: true,
                                excludeEnd: true,
                            },
                        ],
                    }
                ],
            },
            {
                scope: 'number',
                begin: /\b\d+.*_*\b/,
            },
        ],
    };
});

window.onload = function () {
    hljs.highlightAll();
};
