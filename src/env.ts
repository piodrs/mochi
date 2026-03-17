import { loadEnvFile } from "node:process";
import z from "zod";

loadEnvFile();

const envSchema = z.object({
  FIREBASE_PATH: z.string().min(1, "FIREBASE_PATH is required"),
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
