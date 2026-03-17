export interface MemberDocument {
  joinedAt?: Date;
  updatedAt?: Date;
  metrics?: {
    bumps?: {
      total?: number;
      bySource?: {
        disboard?: number;
        discadia?: number;
      };
      lastBumpedAt?: Date;
    };
  };
}
