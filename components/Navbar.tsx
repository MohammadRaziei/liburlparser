import React, { useState } from 'react';
import Link from 'next/link';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faBars } from '@fortawesome/free-solid-svg-icons';
import { faGithub } from '@fortawesome/free-brands-svg-icons';

const Navbar: React.FC = () => {
  const [isMenuOpen, setIsMenuOpen] = useState(false);

  return (
    <nav className="navbar">
      <div className="navbar-container">
        <div className="flex justify-between h-16">
          <div className="flex">
            <div className="flex-shrink-0 flex items-center">
              <Link href="/" className="flex items-center">
                <img 
                  className="h-10 w-auto" 
                  src="https://raw.githubusercontent.com/MohammadRaziei/liburlparser/master/docs/images/logo/liburlparser-logo-1.svg" 
                  alt="liburlparser" 
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
  );
};

export default Navbar;