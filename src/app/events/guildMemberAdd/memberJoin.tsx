import type { EventHandler } from "commandkit";
import {
	ActionRow,
	Button,
	Container,
	Section,
	Separator,
	TextDisplay,
	Thumbnail,
} from "commandkit";
import {
	ButtonStyle,
	channelLink,
	MessageFlags,
	SeparatorSpacingSize,
} from "discord.js";

import { getMemberRecord, upsetMemberRecord } from "@/database/helpers/members";

const { guild, colors } = constants;

const handler: EventHandler<"guildMemberAdd"> = async (member) => {
	const now = new Date();
	const existingMember = await getMemberRecord(member);
	const isReturningMember = Boolean(existingMember.data?.joinedAt);

	await upsetMemberRecord(member, {
		joinedAt: existingMember.data?.joinedAt ?? now,
		updatedAt: now,
	});

	const channelId = guild.channels.welcome;
	const channel = await member.guild.channels.fetch(channelId);

	if (!channel?.isTextBased() || !("send" in channel)) return;

	const title = isReturningMember
		? `${emojis.keqingHeart} Que bom te ver novamente, ${member}!`
		: `${emojis.keqingHeart} Seja bem-vindo, ${member}!`;

	const description = isReturningMember
		? "Você voltou para cá, fico feliz em te ver por aqui de novo."
		: "É um prazer ter você aqui. Esperamos que se sinta em casa.";

	const details = isReturningMember
		? `*Seu registro já estava salvo. Você pode retomar tudo com tranquilidade e acompanhar o que mudou desde a sua última visita.*`
		: `*Antes de começar, vale a pena conferir os canais abaixo para entender a comunidade e descobrir por onde seguir.*`;

	const footer = isReturningMember
		? `-# Para se atualizar, recomendamos começar pelos canais abaixo.`
		: `-# Para começar bem, recomendamos passar primeiro pelos canais abaixo.`;

	await channel.send({
		components: [
			<Container accentColor={colors.highlight}>
				<Section>
					<TextDisplay content={`## ${title}\n${description}\n\n${details}`} />

					<Thumbnail
						url={member.displayAvatarURL()}
						description={`Avatar de ${member.displayName}`}
					/>
				</Section>

				<Separator spacing={SeparatorSpacingSize.Small} />

				<TextDisplay content={footer} />

				<ActionRow>
					<Button
						style={ButtonStyle.Link}
						label="Regras"
						emoji="🔒"
						url={channelLink(guild.channels.rules)}
					/>
					<Button
						style={ButtonStyle.Link}
						label="Bonjour"
						emoji="🍃"
						url={channelLink(guild.channels.bounjour)}
					/>
				</ActionRow>
			</Container>,
		],
		flags: MessageFlags.IsComponentsV2,
	});
};

export default handler;
