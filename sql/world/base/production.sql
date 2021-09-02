-- COMMAND
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('template reload', 3, 'Syntax: .templatenpc reload\nType .templatenpc reload to reload Template NPC database changes');

INSERT INTO `command` (`name`, `security`, `help`) VALUES
('template save alliance', 3, 'Syntax: .template save alliance <spec>\nDumps your current gear and makes it the alliance <spec>s new template.');

INSERT INTO `command` (`name`, `security`, `help`) VALUES
('template save horde', 3, 'Syntax: .template save horde <spec>\nDumps your current gear and makes it the horde <spec>s new template.');

INSERT INTO `command` (`name`, `security`, `help`) VALUES
('template save human', 3, 'Syntax: .template save human <spec>\nDumps your current gear and makes it the human <spec>s new template.');

INSERT INTO `command` (`name`, `security`, `help`) VALUES
('template copy', 3, 'Syntax: .template copy<spec>\nCopies your targets gear onto you.');
