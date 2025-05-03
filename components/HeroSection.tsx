import React from 'react';
import Link from 'next/link';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { IconDefinition } from '@fortawesome/fontawesome-svg-core';
import Container from './Container';

interface NavigationButton {
  href: string;
  label: string;
  icon: React.ReactNode;
  className: string;
  external: boolean;
}

interface HeroSectionProps {
  navigationButtons: NavigationButton[];
}

const HeroSection: React.FC<HeroSectionProps> = ({ navigationButtons }) => {
  return (
    <Container className="py-12 text-center">
      <div className="hero-container">
        <img
          width={180}
          src="https://raw.githubusercontent.com/MohammadRaziei/liburlparser/master/docs/images/logo/liburlparser-logo-1.svg"
          alt="liburlparser Logo"
          className="mb-6 mx-auto"
        />

        <h1 className="text-5xl font-bold text-[#231f20] mb-4">
          liburlparser
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
  );
};

export default HeroSection;