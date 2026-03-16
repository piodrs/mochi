import z from "zod";

const envSchema = z.object({
  MONGO_URI: z.string().min(1, "MONGO_URI is required"),
  DATABASE_NAME: z.string().optional().default("mochi"),
});

const validateEnv = () => {
  const result = envSchema.safeParse(process.env);

  if (!result.success) {
    const errors = result.error.issues
      .map((issue) => `${issue.path.join(".")}: ${issue.message}`)
      .join("\n");

    throw new Error(`Invalid environment variables:\n${errors}`);
  }

  return result.data;
};

export const env = validateEnv();