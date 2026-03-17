import constantsJson from "../constants.json" with { type: "json" };

type RawColors = typeof constantsJson.colors;
type Colors = Record<keyof RawColors, number>;
type GuildConfig = typeof constantsJson.guild;
type Emojis = Record<keyof typeof constantsJson.emojis, string>;

type Constants = Readonly<{
    guild: Readonly<GuildConfig>;
    colors: Readonly<Colors>;
    emojis: Readonly<Emojis>;
}>;

declare global {
    var constants: Constants;
    var emojis: Readonly<Emojis>;
}

function parseColor(hex: string): number {
    return Number.parseInt(hex.replace("#", ""), 16);
}

const colors = Object.freeze(
    Object.fromEntries(
        Object.entries(constantsJson.colors).map(([name, value]) => [name, parseColor(value)]),
    ) as Colors,
);

globalThis.emojis = Object.freeze(
    Object.fromEntries(
        Object.entries(constantsJson.emojis).map(([name, id]) => [
            name,
            `<:${name}:${id}>`,
        ]),
    ) as Emojis,
);

globalThis.constants = Object.freeze({
    ...constantsJson,
    guild: Object.freeze({
        ...constantsJson.guild,
        channels: Object.freeze({ ...constantsJson.guild.channels }),
    }),
    colors,
    emojis: globalThis.emojis,
});

export { };
