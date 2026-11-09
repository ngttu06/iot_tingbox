declare module 'vietqr' {
  export function generateQR(input: {accountNo: string; accountName: string; acqId: string; amount: number; addInfo: string}): Promise<{
    code: string; data: {qrCode: string; qrDataURL: string}
  }>;
}