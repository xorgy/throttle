Name: throttle
Summary: bandwidth limiting pipe
Version: 1.2
Release: 1
URL: http://klicman.org/throttle/
Source: http://klicman.org/throttle/throttle-%{version}.tar.gz
Vendor: James Klicman
Packager: James Klicman <james@klicman.org>
License: GPL v2
Group: Applications/File
BuildRoot: %{_builddir}/%{name}-root

%description
throttle copies the standard input to the standard output while limiting
bandwidth to the specified maximum.


%prep
%setup

%build
%configure
make

%install
%makeinstall


%clean
rm -rf $RPM_BUILD_ROOT
rm -rf %{_builddir}/%{name}-%{version}

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog NEWS README
%{_bindir}/*
%{_mandir}/man*/*


%changelog
* Thu Nov 17 2005 James Klicman <james@klicman.org> 1.2-1
- Updated version number, see ChangeLog for details

* Thu Jul 08 2004 James Klicman <james@klicman.org> 1.1-1
- Initial spec file
