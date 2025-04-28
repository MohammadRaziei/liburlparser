import Head from 'next/head';
import * as React from 'react'; // Keep React import
import Link from 'next/link';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
// Remove faBars if no longer used here, keep others
import { faCode, faTrophy, faLightbulb } from '@fortawesome/free-solid-svg-icons'; 
import { faGithub } from '@fortawesome/free-brands-svg-icons';
import FeaturesSection from '../components/FeaturesSection';
import InstallationGuide from '../components/InstallationGuide';
import UsageExampleGuide from '../components/UsageExampleGuide'; 
import Container from '../components/Container'; 
import Navbar from '../components/Navbar'; 

export default function LibUrlParserDocumentation() {
  // Remove isMenuOpen state
  // const [isMenuOpen, setIsMenuOpen] = React.useState(false); 
  
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
    // Make main a flex column to push footer down
    <main className="bg-gray-50 min-h-screen flex flex-col"> 
      <Head>
        <title>LibUrlParser - Fastest Domain Extractor Library</title>
        <meta name="description" content="Fastest domain extractor library written in C++ with Python binding" />
      </Head>

      <Navbar /> 

      {/* Main content area */}
      {/* Remove items-center, add flex-grow and w-full */}
      <section className="flex-grow w-full"> 
        {/* Hero Section */}
        {/* Container handles width/padding. Added text-center here. */}
        <Container className="py-12 text-center">  
          {/* Removed outer centering/width classes */}
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
        </Container>

        {/* Features Section (already uses Container internally) */}
        <FeaturesSection />

        {/* Performance Section */}
        {/* Container handles width/padding */}
        <Container className="py-12"> 
          {/* Removed section-container class if it existed */}
          <div> 
            <h2 className="section-title">
              <FontAwesomeIcon icon={faTrophy} className="text-[var(--primary-yellow)] mr-3" />
              Performance Comparison
            </h2>
          </div>
        </Container>

        {/* Installation Guide (already uses Container internally) */}
        <InstallationGuide />

        {/* Usage Example Guide (already uses Container internally) */}
        <UsageExampleGuide />

      </section> {/* End main content section */}

      {/* Footer */}
      {/* Container handles width/padding */}
      <footer className="mt-auto pt-16 pb-8 text-center text-gray-500 text-sm w-full bg-[var(--primary-dark)]/5">
        <Container> 
          {/* Removed outer centering/width classes */}
          <div> 
            <p>© {new Date().getFullYear()} LibUrlParser. All rights reserved.</p>
            <p className="mt-2">
              Created by <a href="https://github.com/MohammadRaziei" className="text-[var(--primary-blue)] hover:underline">Mohammad Raziei</a>
            </p>
          </div>
        </Container>
      </footer>
    </main>
  );
}