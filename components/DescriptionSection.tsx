import React from 'react';
import Container from './Container';

const DescriptionSection: React.FC = () => {
  return (
    <Container className="py-8">
      <div className="bg-white rounded-lg shadow-md p-6 max-w-4xl mx-auto">
        <h2 className="text-2xl font-semibold text-[#3871a2] mb-4">Why liburlparser?</h2>
        <div className="text-gray-700">
          <p className="mb-3">
            liburlparser is a simple, lightweight, and fast library for parsing URLs and hosts that:
          </p>
          <ul className="list-disc pl-6 space-y-2 mb-4">
            <li>Extracts components like protocol, host, fragment, query, and path</li>
            <li>Processes hosts to extract subdomain, domain, domain-name, and suffix</li>
            <li>Functions as a top-level domain extractor, correctly identifying that in "ee.aut.ac.ir", the entire "ac.ir" is the suffix, not just ".ir"</li>
            <li>Supports international domain names and non-ASCII characters</li>
            <li>Written in C++ but provides easy-to-use interfaces for Python and Command Line</li>
            <li>Practical and clean design for efficient implementation</li>
          </ul>
        </div>
      </div>
    </Container>
  );
};

export default DescriptionSection;