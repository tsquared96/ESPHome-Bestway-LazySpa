# Contributing to ESPHome Bestway Spa Controller

First off, thank you for considering contributing to this project! 🎉

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates.

**To report a bug:**
1. Use the issue template
2. Include your spa model number
3. Provide your configuration (remove secrets!)
4. Include relevant logs from ESPHome
5. Describe what you expected vs what happened

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When suggesting:
1. Use a clear and descriptive title
2. Provide a detailed description of the proposed enhancement
3. Explain why this enhancement would be useful
4. List any alternative solutions you've considered

### Contributing Code

#### Your First Code Contribution

1. Fork the repository
2. Create a new branch from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. Make your changes
4. Test thoroughly with your hardware
5. Commit with clear messages:
   ```bash
   git commit -m "Add support for model XYZ"
   ```
6. Push to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```
7. Open a Pull Request

#### Pull Request Guidelines

- **Test your code** with actual hardware when possible
- **Update documentation** if you change functionality
- **Follow the existing code style**
- **One feature per PR** - keep them focused
- **Include model number** in PR if model-specific

### Contributing Configurations

If you have a working configuration for a new model:

1. Create a file in `examples/` named after your model
2. Include comments explaining any special requirements
3. Test thoroughly
4. Submit a PR with:
   - The configuration file
   - Update to compatibility list
   - Any special notes in documentation

Example configuration format:
```yaml
# Model: Bestway [Model Name] (NO[Number])
# Tested by: @yourusername
# Date: YYYY-MM-DD
# Special notes: Any peculiarities or requirements

substitutions:
  device_name: layzspa
  model_type: "6wire_2021"  # or appropriate type
  
# ... rest of configuration
```

### Contributing Documentation

Documentation improvements are always welcome! This includes:
- Fixing typos
- Clarifying instructions
- Adding examples
- Translating documentation
- Adding wiring photos/diagrams

## Style Guidelines

### YAML Style
- Use 2 spaces for indentation (no tabs)
- Keep lines under 100 characters when possible
- Comment complex sections
- Group related configuration together

### C++ Style
- Follow the existing code style in `spa_protocol.h`
- Use meaningful variable names
- Comment complex logic
- Keep functions focused and small

### Commit Messages
- Use present tense ("Add feature" not "Added feature")
- Keep first line under 50 characters
- Reference issues and PRs when relevant
- Example:
  ```
  Add support for Bestway Miami model
  
  - Implement 4-wire protocol variant
  - Add model-specific timing adjustments
  - Update compatibility documentation
  
  Fixes #123
  ```

## Testing

### Before Submitting

- [ ] Configuration compiles without errors
- [ ] Tested with actual hardware (if possible)
- [ ] All features work as expected
- [ ] No breaking changes to existing configs
- [ ] Documentation updated if needed

### Testing Checklist

For new model support:
- [ ] Temperature reading works
- [ ] Target temperature can be set
- [ ] Heater control works
- [ ] Filter control works
- [ ] Bubbles control works (if available)
- [ ] Error states are detected
- [ ] Display shows correct values
- [ ] Buttons on physical display still work

## Community

### Getting Help
- Check existing issues and discussions
- Ask in GitHub Discussions
- Provide clear information about your setup

### Code of Conduct
- Be respectful and inclusive
- Welcome newcomers and help them get started
- Focus on what is best for the community
- Show empathy towards other community members

## Recognition

Contributors will be recognized in:
- The README.md contributors section
- Release notes when applicable
- The Hall of Fame (for significant contributions)

## Questions?

Feel free to open a discussion if you have questions about contributing!

Thank you for helping make this project better! 🚀