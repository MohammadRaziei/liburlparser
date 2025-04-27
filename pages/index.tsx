import Head from 'next/head';
import * as React from 'react';
import Link from 'next/link';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faCode, faTrophy, faLightbulb } from '@fortawesome/free-solid-svg-icons';
import { faGithub } from '@fortawesome/free-brands-svg-icons';
import FeaturesSection from '../components/FeaturesSection';

export default function LibUrlParserDocumentation() {
  // Remove the features array since we're moving it to FeaturesSection
  
  const navigationButtons = [
    {
      href: "cpp",
      label: "C++ Documentation",
      icon: <FontAwesomeIcon icon={faCode} />,
      className: "bg-blue-600 hover:bg-blue-700",
      external: false
    },
    {
      href: "python",
      label: "Python Documentation",
      icon: <FontAwesomeIcon icon={faCode} />,
      className: "bg-green-600 hover:bg-green-700",
      external: false
    },
    {
      href: "https://github.com/MohammadRaziei/liburlparser",
      label: "GitHub Repository",
      icon: <FontAwesomeIcon icon={faGithub} />,
      className: "bg-gray-800 hover:bg-gray-900",
      external: true
    }
  ];

  const performanceData = [
    { library: "liburlparser", function: "liburlparser.Host", time: "1.12s", highlight: true },
    { library: "PyDomainExtractor", function: "pydomainextractor.extract", time: "1.50s", highlight: false },
    { library: "publicsuffix2", function: "publicsuffix2.get_sld", time: "9.92s", highlight: false },
    { library: "tldextract", function: "__call__", time: "29.23s", highlight: false },
    { library: "tld", function: "tld.parse_tld", time: "34.48s", highlight: false }
  ];

  return (
    <main className="bg-gray-50">
      <Head>
        <title>LibUrlParser - Fastest Domain Extractor Library</title>
        <meta name="description" content="Fastest domain extractor library written in C++ with Python binding" />
      </Head>

      <section className="layout relative flex min-h-screen flex-col items-center py-12">
        <div className="flex flex-col items-center text-center w-full max-w-4xl">
          <img
            width={200}
            src="https://github.com/MohammadRaziei/liburlparser/raw/master/docs/images/logo/liburlparser-logo-1.svg"
            alt="LibUrlParser Logo"
            className="mb-6"
          />

          <h1 className="text-4xl font-bold text-gray-900">
            LibUrlParser
          </h1>

          <p className="mt-4 text-lg text-gray-600">
            Fastest domain extractor library written in C++ with Python binding.
            First and complete library for parsing URL in C++, Python, and Command Line.
          </p>

          {/* Remove the Feature Highlights section */}

          <div className="mt-8 flex flex-wrap justify-center gap-4">
            {navigationButtons.map((button, index) => {
              const commonClasses = "flex items-center gap-2 rounded-lg px-6 py-3 text-white transition-colors";
              
              return button.external ? (
                <a
                  key={index}
                  href={button.href}
                  target="_blank"
                  rel="noopener noreferrer"
                  className={`${commonClasses} ${button.className}`}
                >
                  {button.icon} {button.label}
                </a>
              ) : (
                <Link
                  key={index}
                  href={button.href}
                  className={`${commonClasses} ${button.className}`}
                >
                  {button.icon} {button.label}
                </Link>
              );
            })}
          </div>
        </div>

        <FeaturesSection />

        {/* Performance Section */}
        <div className="mt-16 w-full max-w-4xl p-8 rounded-xl shadow-sm bg-white">
          <h2 className="text-2xl font-semibold text-gray-800 mb-6">
            Performance Comparison
          </h2>
          <p className="text-gray-600 mb-4">
            LibUrlParser outperforms other domain extraction libraries in both host and URL parsing:
          </p>
          <div className="space-y-6">
            <div>
              <h3 className="font-medium text-gray-800 mb-2">Extract From Host (10 million domains)</h3>
              <div className="overflow-x-auto">
                <table className="min-w-full bg-white border border-gray-200">
                  <thead>
                    <tr className="bg-gray-100">
                      <th className="py-2 px-4 border-b text-left">Library</th>
                      <th className="py-2 px-4 border-b text-left">Function</th>
                      <th className="py-2 px-4 border-b text-left">Time</th>
                    </tr>
                  </thead>
                  <tbody>
                    {performanceData.map((row, index) => (
                      <tr key={index} className={row.highlight ? "bg-green-50" : ""}>
                        <td className="py-2 px-4 border-b font-medium">{row.library}</td>
                        <td className="py-2 px-4 border-b">{row.function}</td>
                        <td className="py-2 px-4 border-b font-bold">{row.time}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>
          </div>
        </div>

        {/* Quick Start Section */}
        <div className="mt-16 w-full max-w-4xl p-8 rounded-xl shadow-sm bg-white">
          <h2 className="text-2xl font-semibold text-gray-800 mb-6">
            Quick Start
          </h2>
          <div className="space-y-6">
            <div>
              <h3 className="font-medium text-gray-800 mb-2">C++ Installation</h3>
              <pre className="bg-gray-100 p-4 rounded-md overflow-x-auto">
                <code>
                  {`# Clone the repository
git clone https://github.com/mohammadraziei/liburlparser
mkdir -p build; cd build
cmake ..
make
sudo make install`}
                </code>
              </pre>
            </div>
            <div>
              <h3 className="font-medium text-gray-800 mb-2">Python Installation</h3>
              <pre className="bg-gray-100 p-4 rounded-md overflow-x-auto">
                <code>
                  {`# Install from PyPI
pip install liburlparser

# Or install from source
pip install git+https://github.com/mohammadraziei/liburlparser.git`}
                </code>
              </pre>
            </div>
            <div>
              <h3 className="font-medium text-gray-800 mb-2">Basic Usage (Python)</h3>
              <pre className="bg-gray-100 p-4 rounded-md overflow-x-auto">
                <code>
                  {`from liburlparser import Url, Host

# Parse URL
url = Url("https://ee.aut.ac.ir/#id")
print(url.suffix, url.domain, url.fragment)

# Parse host
host = Host("ee.aut.ac.ir")
print(host.domain, host.suffix)`}
                </code>
              </pre>
            </div>
          </div>
        </div>

        <footer className="mt-auto pt-12 text-center text-gray-500 text-sm">
          <p>© {new Date().getFullYear()} LibUrlParser. All rights reserved.</p>
          <p className="mt-1">
            Created by <a href="https://github.com/MohammadRaziei" className="text-blue-600 hover:underline">Mohammad Raziei</a>
          </p>
        </footer>
      </section>
    </main>
  );
}