/** @type {import('next').NextConfig} */
const nextConfig = {
  eslint: {
    ignoreDuringBuilds: true,
  },
  typescript: {
    ignoreBuildErrors: true,
  },
  images: {
    unoptimized: true,
  },
  async rewrites() {
    return [
      {
        source: '/api/:path*',
        destination: 'http://localhost:3003/api/:path*',
      },
      {
        source: '/disk/:path*',
        destination: 'http://localhost:3003/disk/:path*',
      },
      {
        source: '/system-disk',
        destination: 'http://localhost:3003/system-disk',
      },
    ]
  },
}

export default nextConfig