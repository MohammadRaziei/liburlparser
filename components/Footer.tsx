import React from 'react';
import Container from './Container';

const Footer: React.FC = () => {
  return (
    <footer className="mt-auto pt-16 pb-8 text-center text-gray-500 text-sm w-full bg-[#231f20]/5">
      <Container>
        <div>
          <p>© {new Date().getFullYear()} liburlparser. All rights reserved.</p>
          <p className="mt-2">
            Created by <a href="https://mohammadraziei.github.io/" className="text-[#3871a2] hover:underline">Mohammad Raziei</a>
          </p>
        </div>
      </Container>
    </footer>
  );
};

export default Footer;