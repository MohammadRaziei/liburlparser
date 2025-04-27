import Head from 'next/head';
import * as React from 'react';
import Link from 'next/link';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faCode, faTrophy, faLightbulb, faBars } from '@fortawesome/free-solid-svg-icons';
import { faGithub } from '@fortawesome/free-brands-svg-icons';
import FeaturesSection from '../components/FeaturesSection';
import InstallationGuide from '../components/InstallationGuide';

export default function LibUrlParserDocumentation() {
  const [isMenuOpen, setIsMenuOpen] = React.useState(false);
  
  const navigationButtons = [
    {
      href: "cpp",
      label: "C++ Documentation",
      icon: <FontAwesomeIcon icon={faCode} />,
      className: "btn-primary",
      external: false
    },
    {
      href: "python",
      label: "Python Documentation",
      icon: <FontAwesomeIcon icon={faCode} />,
      className: "btn-secondary",
      external: false
    },
    {
      href: "https://github.com/MohammadRaziei/liburlparser",
      label: "GitHub Repository",
      icon: <FontAwesomeIcon icon={faGithub} />,
      className: "btn-dark",
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
    <main className="bg-gray-50 min-h-screen">
      <Head>
        <title>LibUrlParser - Fastest Domain Extractor Library</title>
        <meta name="description" content="Fastest domain extractor library written in C++ with Python binding" />
      </Head>

      {/* Navbar */}
      <nav className="navbar">
        <div className="navbar-container">
          <div className="flex justify-between h-16">
            <div className="flex">
              <div className="flex-shrink-0 flex items-center">
                <Link href="/" className="flex items-center">
                  <img 
                    className="h-10 w-auto" 
                    src="https://github.com/MohammadRaziei/liburlparser/raw/master/docs/images/logo/liburlparser-logo-1.svg" 
                    alt="LibUrlParser" 
                  />
                  <span className="ml-3 text-xl font-bold text-[var(--primary-dark)]">LibUrlParser</span>
                </Link>
              </div>
              <div className="hidden sm:ml-6 sm:flex sm:space-x-8">
                <Link href="/cpp" className="navbar-link">
                  C++ Docs
                </Link>
                <Link href="/python" className="navbar-link">
                  Python Docs
                </Link>
                <a href="https://github.com/MohammadRaziei/liburlparser" target="_blank" rel="noopener noreferrer" className="navbar-link">
                  <FontAwesomeIcon icon={faGithub} className="mr-1" /> GitHub
                </a>
              </div>
            </div>
            <div className="-mr-2 flex items-center sm:hidden">
              <button
                onClick={() => setIsMenuOpen(!isMenuOpen)}
                className="inline-flex items-center justify-center p-2 rounded-md text-gray-400 hover:text-gray-500 hover:bg-gray-100 focus:outline-none focus:ring-2 focus:ring-inset focus:ring-[var(--primary-blue)]"
              >
                <span className="sr-only">Open main menu</span>
                <FontAwesomeIcon icon={faBars} className="block h-6 w-6" />
              </button>
            </div>
          </div>
        </div>

        {/* Mobile menu, show/hide based on menu state */}
        <div className={`sm:hidden ${isMenuOpen ? 'block' : 'hidden'}`}>
          <div className="pt-2 pb-3 space-y-1">
            <Link href="/cpp" className="mobile-menu-link">
              C++ Docs
            </Link>
            <Link href="/python" className="mobile-menu-link">
              Python Docs
            </Link>
            <a href="https://github.com/MohammadRaziei/liburlparser" target="_blank" rel="noopener noreferrer" className="mobile-menu-link">
              <FontAwesomeIcon icon={faGithub} className="mr-1" /> GitHub
            </a>
          </div>
        </div>
      </nav>

      <section className="layout relative flex flex-col items-center py-12">
        <div className="flex flex-col items-center text-center w-full max-w-4xl px-4">
          <div className="hero-container">
            <img
              width={180}
              src="https://github.com/MohammadRaziei/liburlparser/raw/master/docs/images/logo/liburlparser-logo-1.svg"
              alt="LibUrlParser Logo"
              className="mb-6 mx-auto"
            />

            <h1 className="text-5xl font-bold text-[var(--primary-dark)] mb-4">
              LibUrlParser
            </h1>

            <p className="mt-4 text-xl text-gray-700 max-w-2xl mx-auto">
              Fastest domain extractor library written in C++ with Python binding.
              First and complete library for parsing URL in C++, Python, and Command Line.
            </p>

            <div className="mt-10 flex flex-wrap justify-center gap-4">
              {navigationButtons.map((button, index) => {
                return button.external ? (
                  <a
                    key={index}
                    href={button.href}
                    target="_blank"
                    rel="noopener noreferrer"
                    className={`btn ${button.className}`}
                  >
                    {button.icon} {button.label}
                  </a>
                ) : (
                  <Link
                    key={index}
                    href={button.href}
                    className={`btn ${button.className}`}
                  >
                    {button.icon} {button.label}
                  </Link>
                );
              })}
            </div>
          </div>
        </div>

        <FeaturesSection />

        {/* Performance Section */}
        <div className="section-container">
          <h2 className="section-title">
            <FontAwesomeIcon icon={faTrophy} className="text-[var(--primary-yellow)] mr-3" />
            Performance Comparison
          </h2>
          <p className="text-gray-700 mb-6">
            LibUrlParser outperforms other domain extraction libraries in both host and URL parsing:
          </p>
          <div className="space-y-6">
            <div>
              <h3 className="font-medium text-[var(--primary-blue)] mb-4">Extract From Host (10 million domains)</h3>
              <div className="overflow-x-auto rounded-lg shadow">
                <table className="performance-table">
                  <thead>
                    <tr className="performance-table-header">
                      <th className="performance-table-cell text-left text-[var(--primary-dark)] font-semibold">Library</th>
                      <th className="performance-table-cell text-left text-[var(--primary-dark)] font-semibold">Function</th>
                      <th className="performance-table-cell text-left text-[var(--primary-dark)] font-semibold">Time</th>
                    </tr>
                  </thead>
                  <tbody>
                    {performanceData.map((row, index) => (
                      <tr key={index} className={row.highlight ? "performance-highlight-row" : ""}>
                        <td className="performance-table-cell font-medium">{row.library}</td>
                        <td className="performance-table-cell">{row.function}</td>
                        <td className="performance-table-cell font-bold">{row.time}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>
          </div>
        </div>

        {/* Replace the Quick Start Section with the InstallationGuide component */}
        <InstallationGuide />

        <footer className="mt-auto pt-16 pb-8 text-center text-gray-500 text-sm w-full bg-[var(--primary-dark)]/5">
          <div className="max-w-4xl mx-auto px-4">
            <p>© {new Date().getFullYear()} LibUrlParser. All rights reserved.</p>
            <p className="mt-2">
              Created by <a href="https://github.com/MohammadRaziei" className="text-[var(--primary-blue)] hover:underline">Mohammad Raziei</a>
            </p>
          </div>
        </footer>
      </section>
    </main>
  );
}