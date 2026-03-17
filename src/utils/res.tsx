import { Container, TextDisplay } from "commandkit";
import { MessageFlags } from "discord.js";
import type {
    InteractionEditReplyOptions,
    InteractionReplyOptions,
    MessageEditOptions,
    MessageReplyOptions,
} from "discord.js";

type ResPayload =
    & InteractionReplyOptions
    & InteractionEditReplyOptions
    & MessageReplyOptions
    & MessageEditOptions;

function build(text: string, color: number, ephemeral = false): ResPayload {
    return {
        components: [
            <Container accentColor={color}>
                <TextDisplay>{text}</TextDisplay>
            </Container>,
        ],
        flags: ephemeral
            ? MessageFlags.IsComponentsV2 | MessageFlags.Ephemeral
            : MessageFlags.IsComponentsV2,
    } as ResPayload;
}

const { colors } = constants;

export const res = {
    info: (text: string, ephemeral = false) => build(text, colors.info, ephemeral),
    success: (text: string, ephemeral = false) => build(text, colors.success, ephemeral),
    warning: (text: string, ephemeral = false) => build(text, colors.warning, ephemeral),
    error: (text: string, ephemeral = false) => build(text, colors.error, ephemeral),
    brand: (text: string, ephemeral = false) => build(text, colors.brand, ephemeral),
} as const;
