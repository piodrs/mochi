import constantsJson from "../constants.json" with { type: "json" };

type Colors = typeof constantsJson.colors;
type Emojis = Record<keyof typeof constantsJson.emojis, string>;

type Constants = Readonly<{
    colors: Readonly<Colors>;
    emojis: Readonly<Emojis>;
}>;

declare global {
    var constants: Constants;
    var emojis: Readonly<Emojis>;
}

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
    colors: Object.freeze({ ...constantsJson.colors }),
    emojis: globalThis.emojis,
});

export { };