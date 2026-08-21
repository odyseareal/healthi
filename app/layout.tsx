import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  metadataBase: new URL("https://pulseboard-fitness.liuguiqing82.chatgpt.site"),
  title: "Healthi",
  description: "A connected fitness, nutrition and wellbeing dashboard for Arduino.",
  openGraph: {
    title: "Healthi",
    description: "Arduino-powered fitness, nutrition and wellbeing.",
    images: [{ url: "/og.png", width: 1200, height: 630, alt: "Healthi fitness dashboard" }],
  },
  twitter: {
    card: "summary_large_image",
    title: "Healthi",
    description: "Arduino-powered fitness, nutrition and wellbeing.",
    images: ["/og.png"],
  },
  icons: {
    icon: "/favicon.svg",
    shortcut: "/favicon.svg",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className="antialiased">{children}</body>
    </html>
  );
}
