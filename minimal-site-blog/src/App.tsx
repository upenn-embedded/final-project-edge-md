import { BrowserRouter, Route, Routes, useLocation } from 'react-router-dom';
import { useEffect } from 'react';
import { BlogLayout } from './components/BlogLayout';
import { ArchitecturePage } from './pages/ArchitecturePage';
import { BuildPage } from './pages/BuildPage';
import { DecisionsPage } from './pages/DecisionsPage';
import { Home } from './pages/Home';
import { NotFound } from './pages/NotFound';
import { TimelinePage } from './pages/TimelinePage';
import { AluPage } from './pages/modules/AluPage';
import { ControlPage } from './pages/modules/ControlPage';
import { DisplayPage } from './pages/modules/DisplayPage';
import { InputPage } from './pages/modules/InputPage';
import { MemoryPage } from './pages/modules/MemoryPage';
import { ShiftPage } from './pages/modules/ShiftPage';

function ScrollTop() {
  const { pathname } = useLocation();
  useEffect(() => {
    window.scrollTo(0, 0);
  }, [pathname]);
  return null;
}

export default function App() {
  return (
    <BrowserRouter>
      <ScrollTop />
      <Routes>
        <Route path="/" element={<BlogLayout />}>
          <Route index element={<Home />} />
          <Route path="decisions" element={<DecisionsPage />} />
          <Route path="timeline" element={<TimelinePage />} />
          <Route path="build" element={<BuildPage />} />
          <Route path="architecture" element={<ArchitecturePage />} />
          <Route path="modules/alu" element={<AluPage />} />
          <Route path="modules/control" element={<ControlPage />} />
          <Route path="modules/memory" element={<MemoryPage />} />
          <Route path="modules/display" element={<DisplayPage />} />
          <Route path="modules/input" element={<InputPage />} />
          <Route path="modules/shift" element={<ShiftPage />} />
          <Route path="*" element={<NotFound />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}
