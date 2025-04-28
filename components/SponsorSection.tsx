import React from 'react';
import Container from './Container';

const SponsorSection: React.FC = () => {
  return (
    <Container className="py-8 text-center"> {/* Adjust padding as needed */}
      <h3 className="text-lg font-medium text-gray-600 mb-4">
        Proudly Sponsored By
      </h3>
      <a 
        href="https://mci.ir/" // Link to MCI's website
        target="_blank" 
        rel="noopener noreferrer"
        className="inline-block"
      >
        <img 
          src="https://upload.wikimedia.org/wikipedia/commons/0/0a/MCI_logo_%282025%29.svg" 
          alt="MCI (Hamrah-e Aval) Logo" 
          className="h-16 mx-auto" // Adjust height as needed
        />
      </a>
    </Container>
  );
};

export default SponsorSection;