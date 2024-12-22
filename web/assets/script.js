const CORN_KEYWORDS = [
    'asm',
    'auto',
    'break',
    'case',
    'const',
    'continue',
    'default',
    'do',
    'else',
    'for',
    'goto',
    'if',
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

    // memory
    'alloc',
    'delete',
    'new',
    'purge',

    // encapsulation
    'external',
    'internal',
    'restrict',

    // reserved
    'private',
    'protected',
    'public',

    // module
    'import',
    'module',
];

const CORN_TYPES = [
    'char',
    'string',
    'utf8',
    'utf16',
    'utf32',

    'float',
    'float16',
    'float32',
    'float64',

    'int',
    'uint',
    'int8',
    'uint8',
    'int16',
    'uint16',
    'int32',
    'uint32',
    'int64',
    'uint64',

    'void',
    'bool',
    'complex',
    'imaginary',

    // abstract
    'enum',
    'struct',
    'contract',
];

const KEYWORDS = {
    keyword: CORN_KEYWORDS,
    type: CORN_TYPES,
    literal: 'true false null',
    built_in: 'std math '
        + 'print printn ',
};

const decimalDigits = '[0-9](_?[0-9])*';
const frac = `\\.(${decimalDigits})`;
const decimalInteger = `0|[1-9](_?[0-9])*|0[0-7]*[89][0-9]*`;

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
                        contains: [
                            {
                                scope: 'doctag',
                                match: /@\w+/, ///@\w+/
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
                variants: [
                    // DecimalLiteral
                    {
                        begin: `(\\b(${decimalInteger})((${frac})|\\.)?|(${frac}))` +
                            `[eE][+-]?(${decimalDigits})\\b`
                    },
                    { begin: `\\b(${decimalInteger})\\b((${frac})\\b|\\.)?|(${frac})\\b` },

                    // DecimalBigIntegerLiteral
                    { begin: `\\b(0|[1-9](_?[0-9])*)n\\b` },

                    // NonDecimalIntegerLiteral
                    { begin: "\\b0[xX][0-9a-fA-F](_?[0-9a-fA-F])*n?\\b" },
                    { begin: "\\b0[bB][0-1](_?[0-1])*n?\\b" },
                    { begin: "\\b0[oO][0-7](_?[0-7])*n?\\b" },

                    // LegacyOctalIntegerLiteral (does not include underscore separators)
                    // https://tc39.es/ecma262/#sec-additional-syntax-numeric-literals
                    { begin: "\\b0[0-7]+n?\\b" },
                ],
            },
        ],
    };
});

hljs.initHighlightingOnLoad();
