import { ContainerBuilder, MessageFlags, TextDisplayBuilder } from "discord.js";
import type {
    InteractionEditReplyOptions,
    InteractionReplyOptions,
    MessageEditOptions,
    MessageReplyOptions,
} from "discord.js";

type ResPayload = InteractionReplyOptions & InteractionEditReplyOptions & MessageReplyOptions & MessageEditOptions;

function build(text: string, color: number, ephemeral = false): ResPayload {
    const container = new ContainerBuilder()
        .setAccentColor(color)
        .addTextDisplayComponents(new TextDisplayBuilder().setContent(text));

    return {
        components: [container],
        flags: ephemeral
            ? MessageFlags.IsComponentsV2 | MessageFlags.Ephemeral
            : MessageFlags.IsComponentsV2,
    } as ResPayload;
}

function hexToInt(hex: string): number {
    return parseInt(hex.replace("#", ""), 16);
}

const { colors } = constants;

export const res = {
    info: (text: string, ephemeral = false) => build(text, hexToInt(colors.info), ephemeral),
    success: (text: string, ephemeral = false) => build(text, hexToInt(colors.success), ephemeral),
    warning: (text: string, ephemeral = false) => build(text, hexToInt(colors.warning), ephemeral),
    error: (text: string, ephemeral = false) => build(text, hexToInt(colors.error), ephemeral),
    brand: (text: string, ephemeral = false) => build(text, hexToInt(colors.brand), ephemeral),
} as const;