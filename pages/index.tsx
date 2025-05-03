import Head from 'next/head';
import * as React from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faCode } from '@fortawesome/free-solid-svg-icons';
import { faGithub } from '@fortawesome/free-brands-svg-icons';
import FeaturesSection from '../components/FeaturesSection';
import InstallationGuide from '../components/InstallationGuide';
import UsageExampleGuide from '../components/UsageExampleGuide';
import Navbar from '../components/Navbar';
import HeroSection from '../components/HeroSection';
import DescriptionSection from '../components/DescriptionSection';
import PerformanceSection from '../components/PerformanceSection';
import Footer from '../components/Footer';
// import SponsorSection from '../components/SponsorSection';

export default function LibUrlParserDocumentation() {
  const navigationButtons = [
    {
      href: "docs/cpp",
      label: "C++ Documentation",
      icon: <FontAwesomeIcon icon={faCode} />,
      className: "btn-secondary",
      external: false
    },
    {
      href: "docs/python",
      label: "Python Documentation",
      icon: <FontAwesomeIcon icon={faCode} />,
      className: "btn-primary",
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
    <main className="bg-gray-50 min-h-screen flex flex-col">
      <Head>
        <title>liburlparser - Fastest Domain Extractor Library</title>
        <meta name="description" content="Fastest domain extractor library written in C++ with Python binding" />
      </Head>

      <Navbar />

      {/* Main content area */}
      <section className="flex-grow w-full">
        <HeroSection navigationButtons={navigationButtons} />
        <DescriptionSection />
        <FeaturesSection />
        {/* <SponsorSection /> */}
        <PerformanceSection performanceData={performanceData} />
        <InstallationGuide />
        <UsageExampleGuide />
      </section>

      <Footer />
    </main>
  );
}